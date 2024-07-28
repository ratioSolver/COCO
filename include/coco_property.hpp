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
     * @param property_fact_builder A pointer to the FactBuilder object.
     * @param value The value to be set for the property.
     */
    virtual void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept = 0;

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

    std::string to_deftemplate_name(const type &tp, bool is_static) const noexcept;

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
  class integer_property final : public property
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
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
  class float_property final : public property
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
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
  class string_property final : public property
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
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
  class symbol_property final : public property
  {
  public:
    /**
     * @brief Constructs a `symbol_property` object with the given name and description.
     *
     * @param name The name of the symbol property.
     * @param description The description of the symbol property.
     * @param default_value The default value for the property.
     * @param values The possible values for the property.
     * @param multiple Indicates whether the property can have multiple values.
     */
    symbol_property(const std::string &name, const std::string &description, std::optional<std::vector<std::string>> default_value = std::nullopt, std::vector<std::string> values = {}, bool multiple = false) noexcept;

    /**
     * @brief Validates the symbol property against the given JSON data and schema references.
     *
     * @param j The JSON data to validate.
     * @param schema_refs The schema references to use for validation.
     * @return `true` if the symbol property is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the symbol property to a JSON object.
     *
     * @return The JSON representation of the symbol property.
     */
    json::json to_json() const noexcept override;

  private:
    std::optional<std::vector<std::string>> default_value; // The default value for the property.
    std::vector<std::string> values;                       // The possible values for the property.
    bool multiple;                                         // Indicates whether the property can have multiple values.
  };

  /**
   * @brief Represents an item property.
   *
   * This class inherits from the base class `property` and provides functionality to define and manipulate item properties.
   */
  class item_property final : public property
  {
  public:
    /**
     * @brief Constructs an `item_property` object.
     *
     * @param name The name of the property.
     * @param description The description of the property.
     * @param tp The type of the property.
     * @param default_value The default value of the property (optional).
     * @param values The possible values for the property.
     * @param multiple Indicates whether the property can have multiple values.
     */
    item_property(const std::string &name, const std::string &description, const type &tp, std::optional<std::vector<std::string>> default_value = std::nullopt, std::vector<std::string> values = {}, bool multiple = false) noexcept;

    /**
     * @brief Validates the property value against the given JSON and schema references.
     *
     * @param j The JSON value to validate.
     * @param schema_refs The schema references to validate against.
     * @return `true` if the property value is valid, `false` otherwise.
     */
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;

    /**
     * @brief Converts the property to a deftemplate string representation.
     *
     * @param tp The type of the property.
     * @param is_static Indicates whether the property is static or not.
     * @return The deftemplate string representation of the property.
     */
    std::string to_deftemplate(const type &tp, bool is_static) const noexcept override;

    /**
     * @brief Sets the value of the property.
     *
     * @param property_fact_builder The FactBuilder object to set the value on.
     * @param value The value to set.
     */
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

  private:
    /**
     * @brief Converts the property to a JSON representation.
     *
     * @return The JSON representation of the property.
     */
    json::json to_json() const noexcept override;

  private:
    const type &tp;                                        // The type of the property.
    std::optional<std::vector<std::string>> default_value; // The default value for the property.
    std::vector<std::string> values;                       // The possible values for the property.
    bool multiple;                                         // Indicates whether the property can have multiple values.
  };

  /**
   * @brief Represents a JSON property.
   *
   * This class inherits from the base class `property` and provides functionality to work with JSON properties.
   */
  class json_property final : public property
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
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
   * @brief Creates a property object from a JSON object.
   *
   * This function takes a JSON object as input and creates a property object based on its contents.
   *
   * @param cc The CoCo core object.
   * @param name The name of the property.
   * @param j The JSON object containing the property data.
   * @return A unique pointer to the created property object.
   */
  std::unique_ptr<property> make_property(coco_core &cc, const std::string &name, const json::json &j);
} // namespace coco
