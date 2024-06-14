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

  class geometry_parameter : public coco_parameter
  {
  public:
    geometry_parameter(const std::string &name) : coco_parameter(name, "geometry") {}

    [[nodiscard]] bool validate(const json::json &value) const override
    {
      if (value.get_type() != json::json_type::object || !value.contains("type"))
        return false;
      auto type = value["type"];
      if (type != "Point" && type != "MultiPoint" && type != "LineString" && type != "MultiLineString" && type != "Polygon" && type != "MultiPolygon" && type != "GeometryCollection")
        return false;
      if ((type == "Point" && !value.contains("coordinates")) || !is_position(value["coordinates"]))
        return false;
      if ((type == "MultiPoint" && !value.contains("coordinates")) || value["coordinates"].get_type() != json::json_type::array)
        for (const auto &v : value["coordinates"].as_array())
          if (!is_position(v))
            return false;
      if ((type == "LineString" && !value.contains("coordinates")) || value["coordinates"].get_type() != json::json_type::array)
        for (const auto &v : value["coordinates"].as_array())
          if (!is_position(v))
            return false;
      if ((type == "MultiLineString" && !value.contains("coordinates")) || value["coordinates"].get_type() != json::json_type::array)
      {
        for (const auto &v : value["coordinates"].as_array())
          if (v.get_type() != json::json_type::array)
            return false;
          else
            for (const auto &vv : v.as_array())
              if (!is_position(vv))
                return false;
      }
      if ((type == "Polygon" && !value.contains("coordinates")) || value["coordinates"].get_type() != json::json_type::array)
      {
        for (const auto &v : value["coordinates"].as_array())
          if (v.get_type() != json::json_type::array)
            return false;
          else
            for (const auto &vv : v.as_array())
              if (!is_position(vv))
                return false;
      }
      if ((type == "MultiPolygon" && !value.contains("coordinates")) || value["coordinates"].get_type() != json::json_type::array)
      {
        for (const auto &v : value["coordinates"].as_array())
          if (v.get_type() != json::json_type::array)
            return false;
          else
            for (const auto &vv : v.as_array())
              if (vv.get_type() != json::json_type::array)
                return false;
              else
                for (const auto &vvv : vv.as_array())
                  if (!is_position(vvv))
                    return false;
      }
      if ((type == "GeometryCollection" && !value.contains("geometries")) || value["geometries"].get_type() != json::json_type::array)
      {
        for (const auto &v : value["geometries"].as_array())
          if (v.get_type() != json::json_type::object || !v.contains("type") || !v.contains("coordinates"))
            return false;
          else if (!validate(v))
            return false;
      }
      return true;
    }

  private:
    bool is_position(const json::json &value) const
    {
      if (value.get_type() != json::json_type::array || value.size() < 2)
        return false;
      for (const auto &v : value.as_array())
        if (v.get_type() != json::json_type::number)
          return false;
      return true;
    }
  };

  const json::json parameter_schema{{"parameter",
                                     {{"oneOf", std::vector<json::json>{
                                                    {{"$ref", "#/components/schemas/integer_parameter"}},
                                                    {{"$ref", "#/components/schemas/real_parameter"}},
                                                    {{"$ref", "#/components/schemas/boolean_parameter"}},
                                                    {{"$ref", "#/components/schemas/symbol_parameter"}},
                                                    {{"$ref", "#/components/schemas/string_parameter"}},
                                                    {{"$ref", "#/components/schemas/array_parameter"}},
                                                    {{"$ref", "#/components/schemas/geometry_parameter"}}}}}}};
  const json::json integer_parameter_schema{{"integer_parameter",
                                             {{"type", "object"},
                                              {"description", "An integer parameter. It has a name and, optionally, a minimum and maximum value."},
                                              {"properties",
                                               {{"name", {{"type", "string"}}},
                                                {"type", {{"type", "string"}, {"enum", {"integer"}}}},
                                                {"min", {{"type", "integer"}}},
                                                {"max", {{"type", "integer"}}}}},
                                              {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json real_parameter_schema{{"real_parameter",
                                          {{"type", "object"},
                                           {"description", "A real parameter. It has a name and, optionally, a minimum and maximum value."},
                                           {"properties",
                                            {{"name", {{"type", "string"}}},
                                             {"type", {{"type", "string"}, {"enum", {"real"}}}},
                                             {"min", {{"type", "number"}}},
                                             {"max", {{"type", "number"}}}}},
                                           {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json boolean_parameter_schema{{"boolean_parameter",
                                             {{"type", "object"},
                                              {"description", "A boolean parameter. It has a name."},
                                              {"properties",
                                               {{"name", {{"type", "string"}}},
                                                {"type", {{"type", "string"}, {"enum", {"boolean"}}}}}},
                                              {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json symbol_parameter_schema{{"symbol_parameter",
                                            {{"type", "object"},
                                             {"description", "A symbolic parameter. It has a name and a list of allowed symbols. If multiple is true, the parameter can have multiple values among the symbols."},
                                             {"properties",
                                              {{"name", {{"type", "string"}}},
                                               {"type", {{"type", "string"}, {"enum", {"symbol"}}}},
                                               {"multiple", {{"type", "boolean"}}},
                                               {"symbols", {{"type", "array"}, {"items", {{"type", "string"}}}}}}},
                                             {"required", {"name", "type", "symbols"}}}}};
  const json::json string_parameter_schema{{"string_parameter",
                                            {{"type", "object"},
                                             {"description", "A string parameter. It has a name."},
                                             {"properties",
                                              {{"name", {{"type", "string"}}},
                                               {"type", {{"type", "string"}, {"enum", {"string"}}}}}},
                                             {"required", std::vector<json::json>{"name", "type"}}}}};
  const json::json array_parameter_schema{{"array_parameter",
                                           {{"type", "object"},
                                            {"description", "An array parameter. It has a name, an array type, and a shape."},
                                            {"properties",
                                             {{"name", {{"type", "string"}}},
                                              {"type", {{"type", "string"}, {"enum", {"array"}}}},
                                              {"array_type", {{"$ref", "#/components/schemas/parameter"}}},
                                              {"shape", {{"type", "array"}, {"items", {{"type", "integer"}}}}}}},
                                            {"required", {"name", "type", "array_type", "shape"}}}}};
  const json::json geometry_parameter_schema{{"geometry_parameter",
                                              {{"oneOf", std::vector<json::json>{
                                                             {"$ref", "#/components/schemas/point"},
                                                             {"$ref", "#/components/schemas/multi_point"},
                                                             {"$ref", "#/components/schemas/line_string"},
                                                             {"$ref", "#/components/schemas/multi_line_string"},
                                                             {"$ref", "#/components/schemas/polygon"},
                                                             {"$ref", "#/components/schemas/multi_polygon"},
                                                             {"$ref", "#/components/schemas/geometry_collection"}}}}}};
  const json::json coordinate_schema{"coordinates",
                                     {{"type", "array"},
                                      {"description", "An array of two or three numbers representing the x, y, and optionally z coordinates of the position."},
                                      {"items", {{"type", "number"}}}}};
  const json::json linear_ring_schema{"linear_ring",
                                      {{"type", "array"},
                                       {"description", "A linear ring is a closed LineString with four or more positions. The first and last positions are equivalent (they represent equivalent points). It MUST follow the right-hand rule with respect to the area it bounds, i.e., exterior rings are counterclockwise, and holes are clockwise."},
                                       {"items", {{"$ref", "#/components/schemas/coordinates"}}}}};
  const json::json point_schema{"point",
                                {{"type", "object"},
                                 {"description", "A point is a position in coordinate space."},
                                 {"properties",
                                  {{"type", {{"type", "string"}, {"enum", {"Point"}}}},
                                   {"coordinates", {{"$ref", "#/components/schemas/coordinates"}}}}}}};
  const json::json multi_point_schema{"multi_point",
                                      {{"type", "object"},
                                       {"properties",
                                        {{"type", {{"type", "string"}, {"enum", {"MultiPoint"}}}},
                                         {"coordinates", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coordinates"}}}}}}}}};
  const json::json line_string_schema{"line_string",
                                      {{"type", "object"},
                                       {"properties",
                                        {{"type", {{"type", "string"}, {"enum", {"LineString"}}}},
                                         {"coordinates", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coordinates"}}}}}}}}};
  const json::json multi_line_string_schema{"multi_line_string",
                                            {{"type", "object"},
                                             {"properties",
                                              {{"type", {{"type", "string"}, {"enum", {"MultiLineString"}}}},
                                               {"coordinates", {{"type", "array"}, {"items", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coordinates"}}}}}}}}}}};
  const json::json polygon_schema{"polygon",
                                  {{"type", "object"},
                                   {"properties",
                                    {{"type", {{"type", "string"}, {"enum", {"Polygon"}}}},
                                     {"coordinates", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/linear_ring"}}}}}}}}};
  const json::json multi_polygon_schema{"multi_polygon",
                                        {{"type", "object"},
                                         {"properties",
                                          {{"type", {{"type", "string"}, {"enum", {"MultiPolygon"}}}},
                                           {"coordinates", {{"type", "array"}, {"items", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/linear_ring"}}}}}}}}}}};
  const json::json geometry_collection_schema{"geometry_collection",
                                              {{"type", "object"},
                                               {"properties",
                                                {{"type", {{"type", "string"}, {"enum", {"GeometryCollection"}}}},
                                                 {"geometries", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/geometry_parameter"}}}}}}}}};
} // namespace coco
