#pragma once

#include "json.hpp"
#include "clips.h"

namespace coco
{
  class type;

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
} // namespace coco
