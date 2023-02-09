#pragma once

#include "sensor.h"
#include "sensor_type.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace coco
{
  class coco_db;

  /**
   * @brief A chart.
   *
   */
  class chart
  {
  public:
    chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label);

    virtual std::string get_type() const = 0;

    const std::string &get_title() const { return title; }
    const std::string &get_x_label() const { return x_label; }
    const std::string &get_y_label() const { return y_label; }

  protected:
    void set_last_update(const std::chrono::milliseconds::rep &time) { last_update = time; }

    virtual json::json to_json() const;
    friend json::json to_json(const chart &c) { return c.to_json(); }

  protected:
    coco_db &db;

  private:
    std::string title;
    std::string x_label;
    std::string y_label;
    std::chrono::milliseconds::rep last_update = 0;
  };

  /**
   * @brief A simple chart with a single series and categories.
   *
   */
  class sc_chart : public chart
  {
  public:
    sc_chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<std::string> &categories = {}, const std::vector<std::string> &series_names = {}, const std::vector<std::vector<double>> &values = {});

    virtual std::string get_type() const override { return "sc_chart"; }

    const std::vector<std::string> &get_categories() const { return categories; }
    const std::vector<std::string> &get_series_names() const { return series_names; }
    const std::vector<std::vector<double>> &get_values() const { return values; }

    void add_category(const std::string &category, const std::vector<double> &vs);
    void add_series(const std::string &series_name, const std::vector<double> &vs);

    double get_value(const std::string &category, const std::string &series_name) const { return values[category_index.at(category)][series_index.at(series_name)]; }
    void set_value(const std::string &category, const std::string &series_name, double value) { values[category_index.at(category)][series_index.at(series_name)] = value; }

  protected:
    virtual json::json to_json() const override;

  private:
    std::vector<std::string> categories;
    std::unordered_map<std::string, size_t> category_index;
    std::vector<std::string> series_names;
    std::unordered_map<std::string, size_t> series_index;
    std::vector<std::vector<double>> values;
  };

  class aggregator_chart : public sc_chart, public sensor_listener
  {
  public:
    aggregator_chart(coco_db &db, const std::string &title, const std::string &x_label, const std::string &y_label, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::chrono::milliseconds::rep &from, const std::chrono::milliseconds::rep &to);
    ~aggregator_chart();

    virtual std::string get_type() const override { return "aggregator_chart"; }

    void new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value);

  private:
    void sensor_name_updated([[maybe_unused]] const sensor &s, [[maybe_unused]] const std::string &name) override {}

    void sensor_value_updated(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &val) override { new_sensor_value(s, time, val); }

    json::json to_json() const override;

  private:
    const sensor_type &type;
    std::unordered_map<std::string, size_t> parameter_index;
    std::vector<std::reference_wrapper<sensor>> sensors;
    std::unordered_map<sensor *, size_t> sensor_index;
    std::chrono::milliseconds::rep from;
    std::chrono::milliseconds::rep to;
  };
} // namespace coco
