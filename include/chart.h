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
    chart(const std::string &title, const std::string &x_label, const std::string &y_label);
    virtual ~chart() = default;

    virtual std::string get_type() const = 0;

    const std::string &get_title() const { return title; }
    const std::string &get_x_label() const { return x_label; }
    const std::string &get_y_label() const { return y_label; }

  protected:
    void set_last_update(const std::chrono::milliseconds::rep &time) { last_update = time; }

    virtual json::json to_json() const;
    friend json::json to_json(const chart &c) { return c.to_json(); }

  private:
    std::string title;
    std::string x_label;
    std::string y_label;
    std::chrono::milliseconds::rep last_update = 0;
  };
  using chart_ptr = utils::u_ptr<chart>;

  class sensor_aggregator
  {
  public:
    sensor_aggregator(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss);

    virtual void new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value) = 0;

  protected:
    coco_db &db;
    const sensor_type &type;
    const std::vector<std::reference_wrapper<sensor>> &sensors;
    std::unordered_map<std::string, size_t> p_idx;
    std::unordered_map<const sensor *, size_t> s_idx;
  };

  /**
   * @brief A simple chart with a single series and categories.
   *
   */
  class sc_chart : public chart
  {
  public:
    sc_chart(const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<std::string> &categories = {}, const std::vector<std::string> &series_names = {}, const std::vector<std::vector<double>> &values = {});

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

  class sensor_aggregator_chart : public sensor_aggregator, public sc_chart
  {
  public:
    sensor_aggregator_chart(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::string &title, const std::string &x_label, const std::string &y_label, const std::chrono::milliseconds::rep &from, const std::chrono::milliseconds::rep &to);

    virtual std::string get_type() const override { return "aggregator_chart"; }

    virtual void new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value) override;

  private:
    json::json to_json() const override;

  private:
    std::chrono::milliseconds::rep from;
    std::chrono::milliseconds::rep to;
  };

  class sensor_adder_chart : public sensor_aggregator_chart
  {
  public:
    sensor_adder_chart(coco_db &db, const sensor_type &type, const std::vector<std::reference_wrapper<sensor>> &ss, const std::string &title, const std::string &x_label, const std::string &y_label, const std::chrono::milliseconds::rep &from, const std::chrono::milliseconds::rep &to);

    virtual std::string get_type() const override { return "adder_chart"; }

    virtual void new_sensor_value(const sensor &s, const std::chrono::milliseconds::rep &time, const json::json &value) override;
  };

  class heat_map_data
  {
  public:
    heat_map_data(const double &x, const double &y, double value);

    const double &get_x() const { return x; }
    const double &get_y() const { return y; }
    double get_value() const { return value; }

  private:
    double x;
    double y;
    double value;
  };

  class heat_map_chart : public chart
  {
  public:
    heat_map_chart(const std::string &title, const std::string &x_label, const std::string &y_label, const std::vector<heat_map_data> &data = {});

    virtual std::string get_type() const override { return "heat_map_chart"; }

    const std::vector<heat_map_data> &get_data() const { return data; }

    void add_data(const heat_map_data &d) { data.push_back(d); }

  private:
    virtual json::json to_json() const override;

  private:
    std::vector<heat_map_data> data;
  };
} // namespace coco