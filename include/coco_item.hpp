#pragma once

#include "json.hpp"
#include "memory.hpp"
#include "clips.h"
#include <chrono>
#include <optional>

namespace coco
{
  class type;

  /**
   * @brief Represents an item.
   *
   * This class provides functionality to create and manage items.
   */
  class item
  {
  public:
    /**
     * @brief Constructs an item object.
     *
     * @param tp The type of the item.
     * @param id The ID of the item.
     * @param props The properties of the item.
     * @param val The value of the item.
     * @param timestamp The timestamp of the value.
     */
    item(const type &tp, std::string_view id, json::json &&props = json::json(), std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val = std::nullopt) noexcept;
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
    [[nodiscard]] const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &get_value() const { return value; }

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
     * This function sets the value of the item using the provided pair of JSON value and timestamp.
     *
     * @param val The pair of JSON value and timestamp.
     */
    void set_value(std::pair<json::json, std::chrono::system_clock::time_point> &&val);

  private:
    const type &tp;                                                                    // The type of the item.
    const std::string id;                                                              // The ID of the item.
    Fact *item_fact = nullptr;                                                         // The fact representing the item.
    Fact *is_instance_of = nullptr;                                                    // The fact representing the type of the item.
    std::map<std::string, Fact *> properties_facts;                                    // The facts representing the properties of the item.
    std::map<std::string, Fact *> value_facts;                                         // The facts representing the value of the item.
    json::json properties;                                                             // The properties of the item.
    std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value; // The value of the item.
  };
} // namespace coco
