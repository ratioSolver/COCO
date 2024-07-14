#pragma once

#include "coco_property.hpp"

namespace coco
{
  class coco_db;

  /**
   * @brief Represents a type in the coco database.
   *
   * The `type` class stores information about a type, including its ID, name, description, and properties.
   * It provides methods to access and retrieve this information.
   */
  class type final
  {
    friend class coco_db;

  public:
    /**
     * @brief Constructs a new `type` object.
     *
     * @param id The ID of the type.
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     */
    type(const std::string &id, const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_properties, std::map<std::string, std::unique_ptr<property>> &&dynamic_properties) noexcept;

    /**
     * @brief Converts the `type` object to a JSON representation.
     *
     * @param t The `type` object to convert.
     * @return The JSON representation of the `type` object.
     */
    friend json::json to_json(const type &t) noexcept;

    /**
     * @brief Gets the ID of the type.
     *
     * @return The ID of the type.
     */
    std::string get_id() const noexcept { return id; }

    /**
     * @brief Gets the name of the type.
     *
     * @return The name of the type.
     */
    std::string get_name() const noexcept { return name; }

    /**
     * @brief Gets the description of the type.
     *
     * @return The description of the type.
     */
    std::string get_description() const noexcept { return description; }

    /**
     * @brief Gets the static properties of the type.
     *
     * @return The static properties of the type.
     */
    const std::map<std::string, std::unique_ptr<property>> &get_static_properties() const noexcept { return static_properties; }

    /**
     * @brief Gets the dynamic properties of the type.
     *
     * @return The dynamic properties of the type.
     */
    const std::map<std::string, std::unique_ptr<property>> &get_dynamic_properties() const noexcept { return dynamic_properties; }

  private:
    /**
     * @brief Converts the `type` object to a JSON representation.
     *
     * @return The JSON representation of the `type` object.
     */
    json::json to_json() const noexcept;

  private:
    std::string id;                                                      // The ID of the type.
    std::string name;                                                    // The name of the type.
    std::string description;                                             // The description of the type.
    std::map<std::string, std::unique_ptr<property>> static_properties;  // The static properties of the type.
    std::map<std::string, std::unique_ptr<property>> dynamic_properties; // The dynamic properties of the type.
  };

  /**
   * Converts a type object to a JSON representation.
   *
   * @param t The type object to convert.
   * @return The JSON representation of the type object.
   */
  inline json::json to_json(const type &t) noexcept { return t.to_json(); }
} // namespace coco
