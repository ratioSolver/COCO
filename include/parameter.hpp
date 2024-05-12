#pragma once

#include "json.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>

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

    virtual bool validate(const json::json &value) const = 0;

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

    bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::number)
        return false;
      long v = value;
      return v >= min && v <= max;
    }

  private:
    const long min, max;
  };

  class real_parameter : public parameter
  {
  public:
    real_parameter(const std::string &name, double min, double max) : parameter(name, parameter_type::Real), min(min), max(max) {}

    [[nodiscard]] double get_min() const { return min; }
    [[nodiscard]] double get_max() const { return max; }

    bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::number)
        return false;
      double v = value;
      return v >= min && v <= max;
    }

  private:
    const double min, max;
  };

  class boolean_parameter : public parameter
  {
  public:
    boolean_parameter(const std::string &name) : parameter(name, parameter_type::Boolean) {}

    bool validate(const json::json &value) const override { return value.get_type() == json::json_type::boolean; }
  };

  class symbol_parameter : public parameter
  {
  public:
    symbol_parameter(const std::string &name, const std::vector<std::string> &symbols, bool multiple = false) : parameter(name, parameter_type::Symbol), symbols(symbols), multiple(multiple) {}

    [[nodiscard]] const std::vector<std::string> &get_symbols() const { return symbols; }
    [[nodiscard]] bool is_multiple() const { return multiple; }

    bool validate(const json::json &value) const override
    {
      if (multiple)
      {
        if (value.get_type() != json::json_type::array)
          return false;
        for (const auto &v : value.as_array())
        {
          if (v.get_type() != json::json_type::string)
            return false;
          if (std::find(symbols.cbegin(), symbols.cend(), static_cast<std::string>(v)) == symbols.end())
            return false;
        }
        return true;
      }
      else
      {
        if (value.get_type() != json::json_type::string)
          return false;
        return std::find(symbols.begin(), symbols.end(), static_cast<std::string>(value)) != symbols.end();
      }
    }

  private:
    const std::vector<std::string> symbols;
    const bool multiple;
  };

  class string_parameter : public parameter
  {
  public:
    string_parameter(const std::string &name) : parameter(name, parameter_type::String) {}

    bool validate(const json::json &value) const override { return value.get_type() == json::json_type::string; }
  };

  class array_parameter : public parameter
  {
  public:
    array_parameter(const std::string &name, std::unique_ptr<parameter> &&type, const std::vector<int> &shape) : parameter(name, parameter_type::Array), type(std::move(type)), shape(shape) {}

    [[nodiscard]] const parameter &as_array_type() const { return *type; }
    [[nodiscard]] const std::vector<int> &get_shape() const { return shape; }

    bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::array)
        return false;
      if (value.size() != static_cast<size_t>(shape[0]))
        return false;
      for (const auto &v : value.as_array())
      {
        if (v.get_type() != json::json_type::array)
          return false;
        if (v.size() != static_cast<size_t>(shape[1]))
          return false;
        for (const auto &vv : v.as_array())
          if (!type->validate(vv))
            return false;
      }
      return true;
    }

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
      res["multiple"] = p.is_multiple();
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

  const json::json parameter_schema{{"parameter",
                                     {{"oneOf", std::vector<json::json>{
                                                    {{"$ref", "#/components/schemas/integer_parameter"}},
                                                    {{"$ref", "#/components/schemas/real_parameter"}},
                                                    {{"$ref", "#/components/schemas/boolean_parameter"}},
                                                    {{"$ref", "#/components/schemas/symbol_parameter"}},
                                                    {{"$ref", "#/components/schemas/string_parameter"}},
                                                    {{"$ref", "#/components/schemas/array_parameter"}}}}}}};
  const json::json integer_parameter_schema{{"integer_parameter",
                                             {{"type", "object"},
                                              {"properties",
                                               {{"name", {{"type", "string"}}},
                                                {"type", {{"type", "string"}, {"enum", {"integer"}}}},
                                                {"min", {{"type", "integer"}}},
                                                {"max", {{"type", "integer"}}}}},
                                              {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json real_parameter_schema{{"real_parameter",
                                          {{"type", "object"},
                                           {"properties",
                                            {{"name", {{"type", "string"}}},
                                             {"type", {{"type", "string"}, {"enum", {"real"}}}},
                                             {"min", {{"type", "number"}}},
                                             {"max", {{"type", "number"}}}}},
                                           {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json boolean_parameter_schema{{"boolean_parameter",
                                             {{"type", "object"},
                                              {"properties",
                                               {{"name", {{"type", "string"}}},
                                                {"type", {{"type", "string"}, {"enum", {"boolean"}}}}}},
                                              {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json symbol_parameter_schema{{"symbol_parameter",
                                            {{"type", "object"},
                                             {"properties",
                                              {{"name", {{"type", "string"}}},
                                               {"type", {{"type", "string"}, {"enum", {"symbol"}}}},
                                               {"multiple", {{"type", "boolean"}}},
                                               {"symbols", {{"type", "array"}, {"items", {{"type", "string"}}}}}}},
                                             {"required", {"name", "type", "symbols"}}}}};
  const json::json string_parameter_schema{{"string_parameter",
                                            {{"type", "object"},
                                             {"properties",
                                              {{"name", {{"type", "string"}}},
                                               {"type", {{"type", "string"}, {"enum", {"string"}}}}}},
                                             {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json array_parameter_schema{{"array_parameter",
                                           {{"type", "object"},
                                            {"properties",
                                             {{"name", {{"type", "string"}}},
                                              {"type", {{"type", "string"}, {"enum", {"array"}}}},
                                              {"array_type", {{"$ref", "#/components/schemas/parameter"}}},
                                              {"shape", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
                                            {"required", {"name", "type", "array_type", "shape"}}}}};
} // namespace coco
