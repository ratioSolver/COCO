#pragma once

#include "json.h"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  class coco_core;
  class coco_db;
  class sensor;

  enum parameter_type
  {
    Integer,
    Float,
    Boolean,
    Symbol,
    String,
    Array
  };

  class parameter
  {
  public:
    parameter(const std::string &name, parameter_type type) : name(name), type(type) {}
    virtual ~parameter() = default;

    const std::string &get_name() const { return name; }
    parameter_type get_type() const { return type; }

  private:
    const std::string name;
    const parameter_type type;
  };
  using parameter_ptr = std::unique_ptr<parameter>;

  class integer_parameter : public parameter
  {
  public:
    integer_parameter(const std::string &name, long min, long max) : parameter(name, parameter_type::Integer), min(min), max(max) {}

    long get_min() const { return min; }
    long get_max() const { return max; }

  private:
    const long min, max;
  };

  class float_parameter : public parameter
  {
  public:
    float_parameter(const std::string &name, double min, double max) : parameter(name, parameter_type::Float), min(min), max(max) {}

    double get_min() const { return min; }
    double get_max() const { return max; }

  private:
    const double min, max;
  };

  class boolean_parameter : public parameter
  {
  public:
    boolean_parameter(const std::string &name) : parameter(name, parameter_type::Boolean) {}
  };

  class symbol_parameter : public parameter
  {
  public:
    symbol_parameter(const std::string &name, const std::vector<std::string> &values) : parameter(name, parameter_type::Symbol), values(values) {}

    const std::vector<std::string> &get_values() const { return values; }

  private:
    const std::vector<std::string> values;
  };

  class string_parameter : public parameter
  {
  public:
    string_parameter(const std::string &name) : parameter(name, parameter_type::String) {}
  };

  class array_parameter : public parameter
  {
  public:
    array_parameter(const std::string &name, parameter_ptr array_type, std::vector<int> dimensions) : parameter(name, parameter_type::Array), array_type(std::move(array_type)), dimensions(std::move(dimensions)) {}

    const parameter_ptr &get_array_type() const { return array_type; }
    const std::vector<int> &get_dimensions() const { return dimensions; }

  private:
    const parameter_ptr array_type;
    const std::vector<int> dimensions;
  };

  class sensor_type
  {
    friend class coco_core;
    friend class coco_db;

  public:
    /**
     * @brief Construct a new sensor type object.
     *
     * @param id the id of the sensor type.
     * @param name the name of the sensor type.
     * @param description the description of the sensor type.
     */
    sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<parameter_ptr> &&pars) : id(id), name(name), description(description)
    {
      for (auto &p : pars)
      {
        if (parameters.find(p->get_name()) != parameters.end())
          throw std::runtime_error("Duplicate parameter name: " + p->get_name());
        parameters.emplace(p->get_name(), std::move(p));
      }
    }
    ~sensor_type() = default;

    /**
     * @brief Get the id of the sensor type.
     *
     * @return const std::string& the id of the sensor type.
     */
    const std::string &get_id() const { return id; }
    /**
     * @brief Get the name of the sensor type.
     *
     * @return const std::string& the name of the sensor type.
     */
    const std::string &get_name() const { return name; }
    /**
     * @brief Get the description of the sensor type.
     *
     * @return const std::string& the description of the sensor type.
     */
    const std::string &get_description() const { return description; }
    /**
     * @brief Get the parameters of the sensor type.
     *
     * @return const std::map<std::string, parameter_ptr>& the parameter types of the sensor type.
     */
    const std::map<std::string, parameter_ptr> &get_parameters() const { return parameters; }
    /**
     * @brief Get the parameter type of the sensor type having the given name.
     *
     * @param name the name of the parameter.
     * @return const parameter& the parameter of the sensor type having the given name.
     */
    const parameter &get_parameter(const std::string &name) const { return *parameters.at(name); }
    /**
     * @brief Check if the sensor type has a parameter having the given name.
     *
     * @param name the name of the parameter.
     * @return true if the sensor type has a parameter having the given name.
     * @return false if the sensor type has no parameter having the given name.
     */
    bool has_parameter(const std::string &name) const { return parameters.find(name) != parameters.end(); }
    /**
     * @brief Get the sensors of the sensor type.
     *
     * @return const std::vector<std::reference_wrapper<sensor>>& the sensors of the sensor type.
     */
    const std::vector<std::reference_wrapper<sensor>> &get_sensors() const { return sensors; }
    /**
     * @brief Get the fact of the sensor type.
     *
     * @return Fact* the fact of the sensor type.
     */
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    std::string name;
    std::string description;
    std::map<std::string, parameter_ptr> parameters;
    std::vector<std::reference_wrapper<sensor>> sensors;
    Fact *fact = nullptr;
  };

  using sensor_type_ptr = std::unique_ptr<sensor_type>;

  inline json::json to_json(const parameter &p)
  {
    json::json j_p{{"name", p.get_name()}};
    switch (p.get_type())
    {
    case parameter_type::Integer:
    {
      const auto &ip = static_cast<const integer_parameter &>(p);
      j_p["type"] = "int";
      if (ip.get_min() != std::numeric_limits<long>::min())
        j_p["min"] = ip.get_min();
      if (ip.get_max() != std::numeric_limits<long>::max())
        j_p["max"] = ip.get_max();
      break;
    }
    case parameter_type::Float:
    {
      const auto &fp = static_cast<const float_parameter &>(p);
      j_p["type"] = "float";
      if (fp.get_min() != std::numeric_limits<double>::min())
        j_p["min"] = fp.get_min();
      if (fp.get_max() != std::numeric_limits<double>::max())
        j_p["max"] = fp.get_max();
      break;
    }
    case parameter_type::Boolean:
    {
      j_p["type"] = "bool";
      break;
    }
    case parameter_type::Symbol:
    {
      const auto &sp = static_cast<const symbol_parameter &>(p);
      j_p["type"] = "symbol";
      if (!sp.get_values().empty())
      {
        json::json j_values(json::json_type::array);
        for (const auto &value : sp.get_values())
          j_values.push_back(value);
        j_p["values"] = j_values;
      }
      break;
    }
    case parameter_type::String:
    {
      j_p["type"] = "string";
      break;
    }
    case parameter_type::Array:
    {
      const auto &ap = static_cast<const array_parameter &>(p);
      j_p["type"] = "array";
      j_p["array_type"] = to_json(*ap.get_array_type());
      json::json j_dims(json::json_type::array);
      for (const auto &dim : ap.get_dimensions())
        j_dims.push_back(dim);
      j_p["dimensions"] = j_dims;
      break;
    }
    }
    return j_p;
  }

  inline json::json to_json(const sensor_type &st)
  {
    json::json j_st{{"id", st.get_id()}, {"name", st.get_name()}, {"description", st.get_description()}};
    json::json j_pars(json::json_type::array);
    for (const auto &[name, type] : st.get_parameters())
      j_pars.push_back(to_json(*type));
    j_st["parameters"] = j_pars;
    return j_st;
  }
} // namespace coco
