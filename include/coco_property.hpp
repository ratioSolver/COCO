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
  class item;

  /**
   * @brief Represents a property with a name and description.
   */
  class property
  {
    friend class coco_core;
    friend class type;
    friend class item;

  public:
    /**
     * @brief Constructs a property with the given name and description.
     *
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     */
    property(const type &tp, const std::string &name, const std::string &description) noexcept;

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
     * Converts the property to a deftemplate string representation.
     *
     * @param tp The type representing the `domain` of the property.
     * @param is_dynamic A flag indicating whether the property is dynamic or static.
     * @return The deftemplate string representation of the property.
     */
    virtual std::string to_deftemplate(bool is_dynamic) const noexcept = 0;

    /**
     * @brief Converts the given type to a deftemplate name.
     *
     * This function takes a type and converts it to a deftemplate name. It optionally
     * allows specifying whether the type is dynamic or not.
     *
     * @param tp The type to convert to a deftemplate name.
     * @param is_dynamic Whether the type is dynamic or not. Default is true.
     * @return The deftemplate name corresponding to the given type.
     */
    std::string to_deftemplate_name(bool is_dynamic = true) const noexcept;

    /**
     * @brief Converts the property to a JSON object.
     * @return The JSON representation of the property.
     */
    virtual json::json to_json() const noexcept;

    friend json::json to_json(const property &p) noexcept;

  private:
    const type &tp;          // The type the property belongs to.
    std::string name;        // The name of the property.
    std::string description; // The description of the property.
  };

  /**
   * @brief Represents a boolean property.
   *
   * This class inherits from the base class `property` and provides functionality for handling boolean properties.
   */
  class boolean_property final : public property
  {
  public:
    /**
     * @brief Constructs a `boolean_property` object with the given name and description.
     *
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     * @param default_value The default value for the property (default: `std::nullopt`).
     */
    boolean_property(const type &tp, const std::string &name, const std::string &description, std::optional<bool> default_value = std::nullopt) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

    json::json to_json() const noexcept override;

  private:
    std::optional<bool> default_value;
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
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     * @param default_value The default value for the property (default: `std::nullopt`).
     * @param min The minimum value allowed for the property (default: `std::numeric_limits<long>::min()`).
     * @param max The maximum value allowed for the property (default: `std::numeric_limits<long>::max()`).
     */
    integer_property(const type &tp, const std::string &name, const std::string &description, std::optional<long> default_value = std::nullopt, long min = std::numeric_limits<long>::min(), long max = std::numeric_limits<long>::max()) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     * @param default_value The default value for the property. Defaults to `std::nullopt`.
     * @param min The minimum value allowed for the property. Defaults to `-std::numeric_limits<double>::max()`.
     * @param max The maximum value allowed for the property. Defaults to `std::numeric_limits<double>::max()`.
     */
    float_property(const type &tp, const std::string &name, const std::string &description, std::optional<double> default_value = std::nullopt, double min = -std::numeric_limits<double>::max(), double max = std::numeric_limits<double>::max()) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
     * @param tp The type the property belongs to.
     * @param name The name of the string property.
     * @param description The description of the string property.
     * @param default_value The default value for the property (default: `std::nullopt`).
     */
    string_property(const type &tp, const std::string &name, const std::string &description, std::optional<std::string> default_value = std::nullopt) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
     * @param tp The type the property belongs to.
     * @param name The name of the symbol property.
     * @param description The description of the symbol property.
     * @param default_value The default value for the property.
     * @param values The possible values for the property.
     * @param multiple Indicates whether the property can have multiple values.
     */
    symbol_property(const type &tp, const std::string &name, const std::string &description, std::optional<std::vector<std::string>> default_value = std::nullopt, std::vector<std::string> values = {}, bool multiple = false) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     * @param domain The type of the property.
     * @param default_value The default value of the property (optional).
     * @param values The possible values for the property.
     * @param multiple Indicates whether the property can have multiple values.
     */
    item_property(const type &tp, const std::string &name, const std::string &description, const type &domain, std::optional<std::vector<std::string>> default_value = std::nullopt, std::vector<std::string> values = {}, bool multiple = false) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

    json::json to_json() const noexcept override;

  private:
    const type &domain;                                    // The domain of the property.
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
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     * @param schema The JSON schema for the property.
     */
    json_property(const type &tp, const std::string &name, const std::string &description, json::json &&schema, std::optional<json::json> default_value = std::nullopt) noexcept;

  private:
    bool validate(const json::json &j, const json::json &schema_refs) const noexcept override;
    std::string to_deftemplate(bool is_dynamic) const noexcept override;
    void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept override;

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
   * @param tp The type the property belongs to.
   * @param name The name of the property.
   * @param j The JSON object containing the property data.
   * @return A unique pointer to the created property object.
   */
  std::unique_ptr<property> make_property(const type &tp, const std::string &name, const json::json &j);
} // namespace coco
