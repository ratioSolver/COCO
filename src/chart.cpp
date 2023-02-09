#include "chart.h"
#include "coco_db.h"
#include <cassert>
#include <algorithm>

namespace coco
{
    chart::chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label) : db(db), title(title), x_label(x_label), y_label(y_label) {}

    json::json chart::to_json() const
    {
        json::json j;
        j["chart_type"] = get_type();
        j["title"] = title;
        j["x_label"] = x_label;
        j["y_label"] = y_label;
        j["last_update"] = last_update;
        return j;
    }

    sc_chart::sc_chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<std::string> &categories, const std::vector<std::string> &series_names, const std::vector<std::vector<double>> &values) : chart(db, title, x_label, y_label), categories(categories), series_names(series_names), values(values)
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
        assert(values.size() == series_names.size());

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

        json::array j_categories;
        for (const auto &c : categories)
            j_categories.push_back(c);
        j["categories"] = std::move(j_categories);

        json::array j_series_names;
        for (const auto &s : series_names)
            j_series_names.push_back(s);
        j["series_names"] = std::move(j_series_names);

        json::array j_values;
        for (const auto &v : values)
        {
            json::array j_v;
            for (const auto &vv : v)
                j_v.push_back(vv);
            j_values.push_back(std::move(j_v));
        }
        j["values"] = std::move(j_values);
        return j;
    }

    aggregator_chart::aggregator_chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::chrono::milliseconds::rep &from, const std::chrono::milliseconds::rep &to) : sc_chart(db, title, x_label, y_label), type(type), sensors(ss), from(from), to(to)
    {
        assert(std::all_of(sensors.begin(), sensors.end(), [&type](const coco::sensor &s)
                           { return &s.get_type() == &type; }));

        size_t j = 0;
        for (auto &[name, type] : type.get_parameters())
        {
            parameter_index[name] = j++;
            add_series(name, {});
        }

        for (size_t i = 0; i < sensors.size(); i++)
        {
            sensor_index[&sensors[i].get()] = i;

            std::vector<double> vs(parameter_index.size(), 0);

            json::array &s_vals = db.get_sensor_values(sensors[i].get(), from, to);
            for (auto &j_vals : s_vals)
            {
                json::object &j_val = j_vals;
                for (auto &[name, j] : parameter_index)
                {
                    json::number_val &j_v = j_val[name];
                    double v = j_v;
                    vs[j] += v;
                }
            }

            add_category(sensors[i].get().get_name(), vs);
        }

        set_last_update(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
    }

    void aggregator_chart::new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value)
    {
        assert(&s.get_type() == &type);

        json::object &j_val = value;
        for (auto &[name, j] : parameter_index)
        {
            json::number_val &j_v = j_val[name];
            double v = j_v;
            set_value(s.get_name(), name, get_value(s.get_name(), name) + v);
        }

        set_last_update(time);
    }

    json::json aggregator_chart::to_json() const
    {
        json::json j = sc_chart::to_json();
        j["sensor_type"] = type.get_name();
        j["from"] = from;
        j["to"] = to;
        return j;
    }
} // namespace coco
