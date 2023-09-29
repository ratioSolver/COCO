#include "chart.h"
#include "coco_db.h"
#include <cassert>
#include <algorithm>

namespace coco
{
    chart::chart(const std::string &title, const std::string &x_label, const std::string &y_label) : title(title), x_label(x_label), y_label(y_label) {}

    json::json chart::to_json() const { return {{"chart_type", get_type()}, {"title", title}, {"x_label", x_label}, {"y_label", y_label}, {"last_update", std::chrono::system_clock::to_time_t(last_update)}}; }

    sensor_aggregator::sensor_aggregator(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss) : db(db), type(type), sensors(ss)
    {
        assert(std::all_of(sensors.begin(), sensors.end(), [&type](const coco::sensor &s)
                           { return &s.get_type() == &type; }));

        size_t j = 0;
        for (auto &[name, type] : type.get_parameters())
            p_idx[name] = j++;

        for (size_t i = 0; i < sensors.size(); i++)
            s_idx[&sensors[i].get()] = i;
    }

    sc_chart::sc_chart(const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<std::string> &categories, const std::vector<std::string> &series_names, const std::vector<std::vector<double>> &values) : chart(title, x_label, y_label), categories(categories), series_names(series_names), values(values)
    {
        assert(categories.size() == values.size());
        assert(std::all_of(values.begin(), values.end(), [&series_names](const std::vector<double> &v)
                           { return v.size() == series_names.size(); }));

        for (size_t i = 0; i < categories.size(); i++)
            category_index[categories[i]] = i;

        for (size_t i = 0; i < series_names.size(); i++)
            series_index[series_names[i]] = i;
    }

    void sc_chart::add_category(const std::string &category, const std::vector<double> &vs)
    {
        assert(vs.size() == series_names.size());

        categories.push_back(category);
        category_index[category] = categories.size() - 1;
        values.push_back(vs);
    }

    void sc_chart::add_series(const std::string &series_name, const std::vector<double> &vs)
    {
        assert(values.size() == categories.size());

        series_names.push_back(series_name);
        series_index[series_name] = series_names.size() - 1;

        for (size_t i = 0; i < vs.size(); i++)
            values[i].push_back(vs[i]);
    }

    json::json sc_chart::to_json() const
    {
        json::json j = chart::to_json();

        json::json j_categories(json::json_type::array);
        for (const auto &c : categories)
            j_categories.push_back(c);
        j["categories"] = std::move(j_categories);

        json::json j_series_names(json::json_type::array);
        for (const auto &s : series_names)
            j_series_names.push_back(s);
        j["series_names"] = std::move(j_series_names);

        json::json j_values(json::json_type::array);
        for (const auto &v : values)
        {
            json::json j_v(json::json_type::array);
            for (const auto &vv : v)
                j_v.push_back(vv);
            json::json jj(std::move(j_v));
            j_values.push_back(std::move(jj));
        }
        j["values"] = std::move(j_values);

        return j;
    }

    sensor_aggregator_chart::sensor_aggregator_chart(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::string &title, const std::string &x_label, const std::string &y_label, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to) : sensor_aggregator(db, type, ss), sc_chart(title, x_label, y_label), from(from), to(to)
    {
        for (auto &[name, type] : type.get_parameters())
            add_series(name, {});

        set_last_update(std::chrono::system_clock::now());
    }

    void sensor_aggregator_chart::new_sensor_value([[maybe_unused]] const sensor &s, const std::chrono::system_clock::time_point &time, [[maybe_unused]] const json::json &value)
    {
        assert(&s.get_type() == &type);

        set_last_update(time);
    }

    json::json sensor_aggregator_chart::to_json() const
    {
        json::json j = sc_chart::to_json();
        j["sensor_type"] = type.get_name();
        j["from"] = std::chrono::system_clock::to_time_t(from);
        j["to"] = std::chrono::system_clock::to_time_t(to);
        return j;
    }

    sensor_adder_chart::sensor_adder_chart(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::string &title, const std::string &x_label, const std::string &y_label, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to) : sensor_aggregator_chart(db, type, ss, title, x_label, y_label, from, to)
    {
        for (size_t i = 0; i < sensors.size(); i++)
        {
            std::vector<double> vs(type.get_parameters().size(), 0);

            json::json s_vals = db.get_sensor_data(sensors[i].get(), from, to);
            for (const auto &j_vals : s_vals.get_array())
                for (auto &[name, j] : p_idx)
                    vs[j] += static_cast<double>(j_vals["value"][name.c_str()]);

            add_category(sensors[i].get().get_name(), vs);
        }
    }

    void sensor_adder_chart::new_sensor_value(const sensor &s, const std::chrono::system_clock::time_point &time, const json::json &value)
    {
        sensor_aggregator_chart::new_sensor_value(s, time, value);

        for (auto &[name, j] : p_idx)
        {
            double v = value[name.c_str()];
            set_value(s.get_name(), name, get_value(s.get_name(), name) + v);
        }
    }

    heat_map_chart::heat_map_chart(const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<heat_map_data> &data) : chart(title, x_label, y_label), data(data) {}

    json::json heat_map_chart::to_json() const
    {
        json::json j = chart::to_json();

        json::json j_data(json::json_type::array);
        for (const auto &d : data)
            j_data.push_back({{"x", d.get_x()}, {"y", d.get_y()}, {"value", d.get_value()}});
        j["data"] = std::move(j_data);

        return j;
    }
} // namespace coco
