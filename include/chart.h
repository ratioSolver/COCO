#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace coco
{
  /**
   * @brief A chart.
   *
   */
  class chart
  {
  public:
    chart(const std::string &title = "", const std::string &x_label = "", const std::string &y_label = "");

    virtual std::string get_type() const = 0;

    const std::string &get_title() const { return title; }
    const std::string &get_x_label() const { return x_label; }
    const std::string &get_y_label() const { return y_label; }

  private:
    std::string title;
    std::string x_label;
    std::string y_label;
  };

  /**
   * @brief A simple chart with a single series and categories.
   *
   */
  class sc_chart : public chart
  {
  public:
    sc_chart(const std::string &title = "", const std::string &x_label = "", const std::string &y_label = "", const std::vector<std::string> &categories = {}, const std::vector<std::string> &series_names = {}, const std::vector<std::vector<double>> &values = {});

    virtual std::string get_type() const override { return "sc_chart"; }

    const std::vector<std::string> &get_categories() const { return categories; }
    const std::vector<std::string> &get_series_names() const { return series_names; }
    const std::vector<std::vector<double>> &get_values() const { return values; }

    void add_category(const std::string &category, const std::vector<double> &vs);
    void add_series(const std::string &series_name, const std::vector<double> &vs);

    double get_value(const std::string &category, const std::string &series_name) const { return values[category_index.at(category)][series_index.at(series_name)]; }
    void set_value(const std::string &category, const std::string &series_name, double value) { values[category_index.at(category)][series_index.at(series_name)] = value; }

  private:
    std::vector<std::string> categories;
    std::unordered_map<std::string, size_t> category_index;
    std::vector<std::string> series_names;
    std::unordered_map<std::string, size_t> series_index;
    std::vector<std::vector<double>> values;
  };
} // namespace coco
