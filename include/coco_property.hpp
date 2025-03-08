#pragma once

#include "json.hpp"
#include "clips.h"
#include <optional>

namespace coco
{
  class type;

  constexpr const char *bool_kw = "bool";
  constexpr const char *int_kw = "int";
  constexpr const char *real_kw = "real";

  /**
   * @brief Represents a property with a name and description.
   */
  class property
  {
  public:
    /**
     * @brief Constructs a property with the given name and description.
     *
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param description The description of the property.
     */
    property(const type &tp, const std::string &name, const std::string &description) noexcept;
    virtual ~property() = default;

    /**
     * @brief Gets the name of the property.
     * @return The name of the property.
     */
    const std::string &get_name() const noexcept { return name; }

    /**
     * @brief Gets the description of the property.
     * @return The description of the property.
     */
    const std::string &get_description() const noexcept { return description; }

    /**
     * @brief Converts the property to a JSON object.
     * @return The JSON representation of the property.
     */
    virtual json::json to_json() const noexcept;

  private:
    /**
     * Sets the value of the property.
     *
     * This function is responsible for setting the value of the property based on the provided `value`.
     *
     * @param property_fact_builder A pointer to the FactBuilder object.
     * @param value The value to be set for the property.
     */
    virtual void set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept = 0;

  private:
    const type &tp;                // The type the property belongs to.
    const std::string name;        // The name of the property.
    const std::string description; // The description of the property.
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
     * @brief Constructs a `boolean_property` object with the given name and a JSON object containing the property data.
     *
     * @param tp The type the property belongs to.
     * @param name The name of the property.
     * @param j The JSON object containing the property data (i.e., an optional description and an optional default value).
     */
    boolean_property(const type &tp, const std::string &name, const json::json &j) noexcept;

  private:
    json::json to_json() const noexcept override;

  private:
    std::optional<bool> default_value;
  };
} // namespace coco
