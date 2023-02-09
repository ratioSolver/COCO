#include "chart.h"
#include <cassert>
#include <algorithm>

namespace coco
{
    chart::chart(const std::string &title, const std::string &x_label, const std::string &y_label) : title(title), x_label(x_label), y_label(y_label) {}

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
} // namespace coco
