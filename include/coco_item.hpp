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
     * @param id The ID of the item.
     * @param tp The type of the item.
     * @param props The properties of the item.
     * @param val The value of the item.
     * @param timestamp The timestamp of the value.
     */
    item(const std::string &id, const type &tp, json::json &&props = json::json(), const json::json &val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) noexcept;
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
     * @brief Gets the properties of the item.
     *
     * @return The properties of the item.
     */
    [[nodiscard]] const json::json &get_properties() const { return properties; }

    /**
     * @brief Gets the value of the item.
     *
     * @return The value of the item.
     */
    [[nodiscard]] const json::json &get_value() const { return value; }

    /**
     * @brief Gets the timestamp of the value.
     *
     * @return The timestamp of the value.
     */
    [[nodiscard]] const std::chrono::system_clock::time_point &get_timestamp() const { return timestamp; }

  private:
    /**
     * @brief Sets the properties of the item.
     *
     * This function takes a JSON object containing the properties and sets them for the item.
     *
     * @param props The JSON object containing the properties.
     */
    void set_properties(json::json &&props);

    /**
     * @brief Sets the value of the item.
     *
     * This function sets the value of the item using the provided JSON value.
     *
     * @param val The JSON value to set as the item's value.
     * @param timestamp The timestamp of the value.
     */
    void set_value(const json::json &val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());

  private:
    Fact *item_fact = nullptr;                       // The fact representing the item.
    Fact *is_instance_of = nullptr;                  // The fact representing the type of the item.
    const std::string id;                            // The ID of the item.
    const type &tp;                                  // The type of the item.
    json::json properties;                           // The properties of the item.
    std::map<std::string, Fact *> property_facts;    // The facts representing the properties of the item.
    json::json value;                                // The value of the item.
    std::map<std::string, Fact *> value_facts;       // The facts representing the values of the properties of the item.
    std::chrono::system_clock::time_point timestamp; // The timestamp of the value.
  };
} // namespace coco
