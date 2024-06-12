#pragma once

#include "json.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include <limits>
#include <algorithm>

namespace coco
{
  /**
   * @brief Represents a parameter.
   *
   * This class represents a parameter with a name and a type.
   */
  class coco_parameter
  {
  public:
    /**
     * @brief Constructs a parameter with the given name and type.
     *
     * @param name The name of the parameter.
     * @param type The type of the parameter.
     */
    coco_parameter(const std::string &name, const std::string &type) : name(name), type(type) {}

    /**
     * @brief Destructor for the parameter.
     */
    virtual ~coco_parameter() = default;

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
    [[nodiscard]] const std::string &get_type() const { return type; }

    virtual bool validate(const json::json &value) const = 0;

    virtual json::json to_json() const { return json::json{{"name", name}, {"type", type}}; };

  private:
    /**
     * @brief Outputs the parameter to the given output stream.
     *
     * @param os The output stream.
     * @return The output stream.
     */
    std::ostream &operator<<(std::ostream &os) const
    {
      os << "Parameter: " << name << " (" << type << ")";
      return os;
    }

  private:
    std::string name; // The name of the parameter.
    std::string type; // The type of the parameter.
  };

  class integer_parameter : public coco_parameter
  {
  public:
    integer_parameter(const std::string &name, long min, long max) : coco_parameter(name, "integer"), min(min), max(max) {}

    [[nodiscard]] long get_min() const { return min; }
    [[nodiscard]] long get_max() const { return max; }

    [[nodiscard]] bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::number)
        return false;
      long v = value;
      return v >= min && v <= max;
    }

    [[nodiscard]] json::json to_json() const override
    {
      json::json res = coco_parameter::to_json();
      if (min != std::numeric_limits<long>::min())
        res["min"] = min;
      if (max != std::numeric_limits<long>::max())
        res["max"] = max;
      return res;
    }

  private:
    const long min, max;
  };

  class real_parameter : public coco_parameter
  {
  public:
    real_parameter(const std::string &name, double min, double max) : coco_parameter(name, "real"), min(min), max(max) {}

    [[nodiscard]] double get_min() const { return min; }
    [[nodiscard]] double get_max() const { return max; }

    [[nodiscard]] bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::number)
        return false;
      double v = value;
      return v >= min && v <= max;
    }

    [[nodiscard]] json::json to_json() const override
    {
      json::json res = coco_parameter::to_json();
      if (min != -std::numeric_limits<double>::infinity())
        res["min"] = min;
      if (max != std::numeric_limits<double>::infinity())
        res["max"] = max;
      return res;
    }

  private:
    const double min, max;
  };

  class boolean_parameter : public coco_parameter
  {
  public:
    boolean_parameter(const std::string &name) : coco_parameter(name, "boolean") {}

    [[nodiscard]] bool validate(const json::json &value) const override { return value.get_type() == json::json_type::boolean; }
  };

  class symbolic_parameter : public coco_parameter
  {
  public:
    symbolic_parameter(const std::string &name, const std::vector<std::string> &symbols, bool multiple = false) : coco_parameter(name, "symbol"), symbols(symbols), multiple(multiple) {}

    [[nodiscard]] const std::vector<std::string> &get_symbols() const { return symbols; }
    [[nodiscard]] bool is_multiple() const { return multiple; }

    [[nodiscard]] bool validate(const json::json &value) const override
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

    [[nodiscard]] json::json to_json() const override
    {
      json::json res = coco_parameter::to_json();
      res["multiple"] = multiple;
      if (!symbols.empty())
      {
        json::json s(json::json_type::array);
        for (const auto &symbol : symbols)
          s.push_back(symbol);
        res["symbols"] = std::move(s);
      }
      return res;
    }

  private:
    const std::vector<std::string> symbols;
    const bool multiple;
  };

  class string_parameter : public coco_parameter
  {
  public:
    string_parameter(const std::string &name) : coco_parameter(name, "string") {}

    [[nodiscard]] bool validate(const json::json &value) const override { return value.get_type() == json::json_type::string; }
  };

  class array_parameter : public coco_parameter
  {
  public:
    array_parameter(const std::string &name, std::unique_ptr<coco_parameter> &&type, const std::vector<int> &shape) : coco_parameter(name, "array"), type(std::move(type)), shape(shape) {}

    [[nodiscard]] const coco_parameter &as_array_type() const { return *type; }
    [[nodiscard]] const std::vector<int> &get_shape() const { return shape; }

    [[nodiscard]] bool validate(const json::json &value) const override
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

    [[nodiscard]] json::json to_json() const override
    {
      json::json res = coco_parameter::to_json();
      res["array_type"] = type->to_json();
      json::json s(json::json_type::array);
      for (const auto &sh : shape)
        s.push_back(sh);
      res["shape"] = std::move(s);
      return res;
    }

  private:
    const std::unique_ptr<coco_parameter> type;
    const std::vector<int> shape;
  };

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
