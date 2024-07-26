#pragma once

#include "coco_type.hpp"

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
    coco_core &cc;                                // The CoCo core object.
    Fact *item_fact = nullptr;                    // The fact representing the item.
    Fact *is_instance_of = nullptr;               // The fact representing the type of the item.
    const std::string id;                         // The ID of the item.
    const type &tp;                               // The type of the item.
    std::string name;                             // The name of the item.
    json::json properties;                        // The properties of the item.
    std::map<std::string, Fact *> property_facts; // The facts representing the properties of the item.
  };
} // namespace coco
