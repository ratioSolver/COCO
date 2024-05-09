#pragma once

#include "json.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <limits>

namespace coco
{
  enum parameter_type
  {
    Integer,
    Real,
    Boolean,
    Symbol,
    String,
    Array
  };

  inline std::string to_string(parameter_type type)
  {
    switch (type)
    {
    case Integer:
      return "Integer";
    case Real:
      return "Real";
    case Boolean:
      return "Boolean";
    case Symbol:
      return "Symbol";
    case String:
      return "String";
    case Array:
      return "Array";
    default:
      return "Unknown";
    }
  }

  /**
   * @brief Represents a parameter.
   *
   * This class represents a parameter with a name and a type.
   */
  class parameter
  {
  public:
    /**
     * @brief Constructs a parameter with the given name and type.
     *
     * @param name The name of the parameter.
     * @param type The type of the parameter.
     */
    parameter(const std::string &name, parameter_type type) : name(name), type(type) {}

    /**
     * @brief Destructor for the parameter.
     */
    virtual ~parameter() = default;

    /**
     * @brief Gets the name of the parameter.
     *
     * @return The name of the parameter.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the type of the parameter.
     *
     * @return The type of the parameter.
     */
    [[nodiscard]] parameter_type get_type() const { return type; }

  private:
    /**
     * @brief Outputs the parameter to the given output stream.
     *
     * @param os The output stream.
     * @return The output stream.
     */
    std::ostream &operator<<(std::ostream &os) const
    {
      os << "Parameter: " << name << " (" << to_string(type) << ")";
      return os;
    }

  private:
    std::string name;    ///< The name of the parameter.
    parameter_type type; ///< The type of the parameter.
  };

  class integer_parameter : public parameter
  {
  public:
    integer_parameter(const std::string &name, long min, long max) : parameter(name, parameter_type::Integer), min(min), max(max) {}

    [[nodiscard]] long get_min() const { return min; }
    [[nodiscard]] long get_max() const { return max; }

  private:
    const long min, max;
  };

  class real_parameter : public parameter
  {
  public:
    real_parameter(const std::string &name, double min, double max) : parameter(name, parameter_type::Real), min(min), max(max) {}

    [[nodiscard]] double get_min() const { return min; }
    [[nodiscard]] double get_max() const { return max; }

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
    symbol_parameter(const std::string &name, const std::vector<std::string> &symbols) : parameter(name, parameter_type::Symbol), symbols(symbols) {}

    [[nodiscard]] const std::vector<std::string> &get_symbols() const { return symbols; }

  private:
    const std::vector<std::string> symbols;
  };

  class string_parameter : public parameter
  {
  public:
    string_parameter(const std::string &name) : parameter(name, parameter_type::String) {}
  };

  class array_parameter : public parameter
  {
  public:
    array_parameter(const std::string &name, std::unique_ptr<parameter> &&type, const std::vector<int> &shape) : parameter(name, parameter_type::Array), type(std::move(type)), shape(shape) {}

    [[nodiscard]] const parameter &as_array_type() const { return *type; }
    [[nodiscard]] const std::vector<int> &get_shape() const { return shape; }

  private:
    const std::unique_ptr<parameter> type;
    const std::vector<int> shape;
  };

  /**
   * Converts a parameter object to a JSON object.
   *
   * @param par The parameter object to convert.
   * @return The JSON object representing the parameter.
   */
  inline json::json to_json(const parameter &par)
  {
    json::json res{{"name", par.get_name()}, {"type", to_string(par.get_type())}};
    switch (par.get_type())
    {
    case Integer:
    {
      auto &p = static_cast<const integer_parameter &>(par);
      if (p.get_min() != std::numeric_limits<long>::min())
        res["min"] = p.get_min();
      if (p.get_max() != std::numeric_limits<long>::max())
        res["max"] = p.get_max();
      break;
    }
    case Real:
    {
      auto &p = static_cast<const real_parameter &>(par);
      if (p.get_min() != -std::numeric_limits<double>::infinity())
        res["min"] = p.get_min();
      if (p.get_max() != std::numeric_limits<double>::infinity())
        res["max"] = p.get_max();
      break;
    }
    case Symbol:
    {
      auto &p = static_cast<const symbol_parameter &>(par);
      if (!p.get_symbols().empty())
      {
        json::json symbols;
        for (const auto &s : p.get_symbols())
          symbols.push_back(s);
        res["symbols"] = std::move(symbols);
      }
      break;
    }
    case Array:
    {
      auto &p = static_cast<const array_parameter &>(par);
      res["array_type"] = to_json(p.as_array_type());
      json::json shape;
      for (const auto &s : p.get_shape())
        shape.push_back(s);
      res["shape"] = std::move(shape);
      break;
    }
    default:
      break;
    }
    return res;
  }
} // namespace coco
