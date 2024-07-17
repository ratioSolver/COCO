#pragma once

#include "json.hpp"
#include "clips.h"
#include <limits>
#include <optional>
#include <memory>

namespace coco
{
  class coco_core;
  class type;

  /**
   * @brief Represents a property with a name and description.
   */
  class property
  {
  public:
    /**
     * @brief Constructs a property with the given name and description.
     * @param name The name of the property.
     * @param description The description of the property.
     */
    property(const std::string &name, const std::string &description) noexcept;

    /**
     * @brief Default destructor.
     */
    virtual ~property() = default;

    /**
     * @brief Gets the name of the property.
     * @return The name of the property.
     */
    std::string get_name() const noexcept { return name; }

    /**
     * @brief Gets the description of the property.
     * @return The description of the property.
     */
    std::string get_description() const noexcept { return description; }

    /**
     * Converts the property to a deftemplate string representation.
     *
     * @param tp The type representing the `domain` of the property.
     * @param is_static A flag indicating whether the property is static or dynamic.
     * @return The deftemplate string representation of the property.
     */
    virtual std::string to_deftemplate(const type &tp, bool is_static) const noexcept = 0;

    /**
     * Sets the value of the property.
     *
     * This function is responsible for setting the value of the property based on the provided `value`.
     *
     * @param item_fact_builder A pointer to the FactBuilder object.
     * @param value The value to be set for the property.
     */
    virtual void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept = 0;

    /**
     * @brief Validates the property against a JSON object and schema references.
     * @param j The JSON object to validate.
     * @param schema_refs The schema references to use for validation.
     * @return True if the property is valid, false otherwise.
     */
    virtual bool validate(const json::json &j, const json::json &schema_refs) const noexcept = 0;

  protected:
    /**
     * @brief Converts the property to a JSON object.
     * @return The JSON representation of the property.
     */
    virtual json::json to_json() const noexcept;

    friend json::json to_json(const property &p) noexcept;

  private:
    std::string name;        // The name of the property.
    std::string description; // The description of the property.
  };

  /**
   * @brief Represents an integer property.
   *
   * This class inherits from the base class `property` and provides functionality for handling integer properties.
   */
  class integer_property : public property
  {
  public:
    /**
     * @brief Constructs an `integer_property` object with the given name, description, minimum and maximum values.
     *
     * @param name The name of the property.
     * @param description The description of the property.
     * @param default_value The default value for the property (default: `std::nullopt`).
     * @param min The minimum value allowed for the property (default: `std::numeric_limits<long>::min()`).
     * @param max The maximum value allowed for the property (default: `std::numeric_limits<long>::max()`).
     */
    integer_property(const std::string &name, const std::string &description, std::optional<long> default_value = std::nullopt, long min = std::numeric_limits<long>::min(), long max = std::numeric_limits<long>::max()) noexcept;

    /**
     * @brief Validates the given JSON object against the property's schema references.
     *
     * @param j The JSON object to validate.
     * @param schema_refs The schema references to use for validation.
     * @return `true` if the JSON object is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the `integer_property` object to a JSON object.
     *
     * @return The JSON representation of the `integer_property` object.
     */
    json::json to_json() const noexcept override;

  private:
    std::optional<long> default_value; // The default value for the property.
    long min;                          // The minimum value allowed for the property.
    long max;                          // The maximum value allowed for the property.
  };

  /**
   * @brief Represents a float property.
   *
   * This class inherits from the base class `property` and provides functionality for handling float properties.
   */
  class float_property : public property
  {
  public:
    /**
     * @brief Constructs a `float_property` object with the given name, description, minimum value, and maximum value.
     *
     * @param name The name of the property.
     * @param description The description of the property.
     * @param default_value The default value for the property. Defaults to `std::nullopt`.
     * @param min The minimum value allowed for the property. Defaults to `-std::numeric_limits<double>::max()`.
     * @param max The maximum value allowed for the property. Defaults to `std::numeric_limits<double>::max()`.
     */
    float_property(const std::string &name, const std::string &description, std::optional<double> default_value = std::nullopt, double min = -std::numeric_limits<double>::max(), double max = std::numeric_limits<double>::max()) noexcept;

    /**
     * @brief Validates the given JSON object against the property's schema references.
     *
     * @param j The JSON object to validate.
     * @param schema_refs The schema references to use for validation.
     * @return `true` if the JSON object is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the `float_property` object to a JSON object.
     *
     * @return The JSON representation of the `float_property` object.
     */
    json::json to_json() const noexcept override;

  private:
    std::optional<double> default_value; // The default value for the property.
    double min;                          // The minimum value allowed for the property.
    double max;                          // The maximum value allowed for the property.
  };

  /**
   * @brief Represents a string property.
   *
   * This class inherits from the base class `property` and provides functionality for handling string properties.
   */
  class string_property : public property
  {
  public:
    /**
     * @brief Constructs a `string_property` object with the given name and description.
     *
     * @param name The name of the string property.
     * @param description The description of the string property.
     */
    string_property(const std::string &name, const std::string &description, std::optional<std::string> default_value = std::nullopt) noexcept;

    /**
     * @brief Validates the string property against the given JSON data and schema references.
     *
     * This function checks if the string property is valid based on the provided JSON data and schema references.
     *
     * @param j The JSON data to validate against.
     * @param schema_refs The schema references to use for validation.
     * @return `true` if the string property is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the string property to a JSON object.
     *
     * This function converts the string property to a JSON object.
     *
     * @return The JSON representation of the string property.
     */
    json::json to_json() const noexcept override;

  private:
    std::optional<std::string> default_value; // The default value for the property.
  };

  /**
   * @brief Represents a symbol property.
   *
   * This class inherits from the base class `property` and provides functionality to handle symbol properties.
   */
  class symbol_property : public property
  {
  public:
    /**
     * @brief Constructs a `symbol_property` object with the given name and description.
     *
     * @param name The name of the symbol property.
     * @param description The description of the symbol property.
     */
    symbol_property(const std::string &name, const std::string &description, std::optional<std::string> default_value = std::nullopt) noexcept;

    /**
     * @brief Validates the symbol property against the given JSON data and schema references.
     *
     * @param j The JSON data to validate.
     * @param schema_refs The schema references to use for validation.
     * @return `true` if the symbol property is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the symbol property to a JSON object.
     *
     * @return The JSON representation of the symbol property.
     */
    json::json to_json() const noexcept override;

  private:
    std::optional<std::string> default_value; // The default value for the property.
  };

  /**
   * @brief Represents a JSON property.
   *
   * This class inherits from the base class `property` and provides functionality to work with JSON properties.
   */
  class json_property : public property
  {
  public:
    /**
     * @brief Constructs a `json_property` object.
     *
     * @param name The name of the property.
     * @param description The description of the property.
     * @param schema The JSON schema for the property.
     */
    json_property(const std::string &name, const std::string &description, json::json &&schema, std::optional<json::json> default_value = std::nullopt) noexcept;

    /**
     * @brief Validates the given JSON object against the JSON schema.
     *
     * @param j The JSON object to validate.
     * @param schema_refs The JSON schema references.
     * @return `true` if the JSON object is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the `json_property` object to a JSON object.
     *
     * @return The JSON representation of the `json_property` object.
     */
    json::json to_json() const noexcept override;

  private:
    json::json schema;                       // The JSON schema for the property.
    std::optional<json::json> default_value; // The default value for the property.
  };

  /**
   * Converts a property object to a JSON representation.
   *
   * @param p The property object to convert.
   * @return The JSON representation of the property.
   */
  inline json::json to_json(const property &p) noexcept { return p.to_json(); }

  /**
   * @brief Creates a property object from a JSON object.
   *
   * This function takes a JSON object as input and creates a property object based on its contents.
   *
   * @param j The JSON object containing the property data.
   * @return A unique pointer to the created property object.
   */
  std::unique_ptr<property> make_property(const json::json &j);

  const json::json property_schema{{"property",
                                    {{"oneOf", std::vector<json::json>{
                                                   {"$ref", "#/components/schemas/int_property"},
                                                   {"$ref", "#/components/schemas/float_property"},
                                                   {"$ref", "#/components/schemas/string_property"},
                                                   {"$ref", "#/components/schemas/symbol_property"},
                                                   {"$ref", "#/components/schemas/json_property"}}}}}};
  const json::json integer_property_schema{{"integer_property",
                                            {{"type", "object"},
                                             {"properties",
                                              {{"type", {{"type", "string"}, {"enum", {"integer"}}}},
                                               {"name", {{"type", "string"}}},
                                               {"description", {{"type", "string"}}},
                                               {"min", {{"type", "integer"}}},
                                               {"max", {{"type", "integer"}}}}},
                                             {"required", std::vector<json::json>{"type", "name"}}}}};
  const json::json float_property_schema{{"float_property",
                                          {{"type", "object"},
                                           {"properties",
                                            {{"type", {{"type", "string"}, {"enum", {"float"}}}},
                                             {"name", {{"type", "string"}}},
                                             {"description", {{"type", "string"}}},
                                             {"min", {{"type", "number"}}},
                                             {"max", {{"type", "number"}}}}},
                                           {"required", std::vector<json::json>{"type", "name"}}}}};
  const json::json string_property_schema{{"string_property",
                                           {{"type", "object"},
                                            {"properties",
                                             {{"type", {{"type", "string"}, {"enum", {"string"}}}},
                                              {"name", {{"type", "string"}}},
                                              {"description", {{"type", "string"}}}}},
                                            {"required", std::vector<json::json>{"type", "name"}}}}};
  const json::json symbol_property_schema{{"symbol_property",
                                           {{"type", "object"},
                                            {"properties",
                                             {{"type", {{"type", "string"}, {"enum", {"symbol"}}}},
                                              {"name", {{"type", "string"}}},
                                              {"description", {{"type", "string"}}}}},
                                            {"required", std::vector<json::json>{"type", "name"}}}}};
  const json::json json_property_schema{{"json_property",
                                         {{"type", "object"},
                                          {"properties",
                                           {{"type", {{"type", "string"}, {"enum", {"json"}}}},
                                            {"name", {{"type", "string"}}},
                                            {"description", {{"type", "string"}}},
                                            {"schema", {{"type", "object"}}}}},
                                          {"required", std::vector<json::json>{"type", "name", "schema"}}}}};
} // namespace coco
