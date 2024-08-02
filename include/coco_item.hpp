#pragma once

#include "coco_type.hpp"
#include <chrono>

namespace coco
{
  /**
   * @brief Represents an item.
   *
   * This class provides functionality to create and manage items.
   */
  class item final
  {
    friend class coco_db;
    friend class coco_core;

  public:
    /**
     * @brief Constructs an item object.
     *
     * @param cc The CoCo core object.
     * @param id The ID of the item.
     * @param tp The type of the item.
     * @param name The name of the item.
     * @param props The properties of the item.
     */
    item(coco_core &cc, const std::string &id, const type &tp, const std::string &name, const json::json &props) noexcept;
    ~item() noexcept;

    /**
     * @brief Gets the ID of the item.
     *
     * @return The ID of the item.
     */
    [[nodiscard]] const std::string &get_id() const { return id; }

    /**
     * @brief Gets the type of the item.
     *
     * @return The type of the item.
     */
    [[nodiscard]] const type &get_type() const { return tp; }

    /**
     * @brief Gets the name of the item.
     *
     * @return The name of the item.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the properties of the item.
     *
     * @return The properties of the item.
     */
    [[nodiscard]] const json::json &get_properties() const { return properties; }

  private:
    /**
     * @brief Sets the name of the item.
     *
     * This function sets the name of the item to the specified value.
     *
     * @param name The new name for the item.
     */
    void set_name(const std::string &name);

    /**
     * @brief Sets the properties of the item.
     *
     * This function takes a JSON object containing the properties and sets them for the item.
     *
     * @param props The JSON object containing the properties.
     */
    void set_properties(const json::json &props);

    /**
     * @brief Sets the value of the item.
     *
     * This function sets the value of the item using the provided JSON value.
     *
     * @param value The JSON value to set as the item's value.
     * @param timestamp The timestamp of the value.
     */
    void set_value(const json::json &value, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());

  private:
    coco_core &cc;                                // The CoCo core object.
    Fact *item_fact = nullptr;                    // The fact representing the item.
    Fact *is_instance_of = nullptr;               // The fact representing the type of the item.
    const std::string id;                         // The ID of the item.
    const type &tp;                               // The type of the item.
    std::set<const type *> types;                 // The types of the item.
    std::string name;                             // The name of the item.
    json::json properties;                        // The properties of the item.
    std::map<std::string, Fact *> property_facts; // The facts representing the properties of the item.
    std::map<std::string, Fact *> value_facts;    // The facts representing the values of the properties of the item.
  };
} // namespace coco
