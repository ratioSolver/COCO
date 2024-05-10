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
  public:
    /**
     * @brief Constructs an item object.
     *
     * @param id The ID of the item.
     * @param tp The type of the item.
     * @param name The name of the item.
     * @param data The data associated with the item (optional).
     */
    item(const std::string &id, const type &tp, const std::string &name, json::json &&data = {});

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
     * @brief Gets the data associated with the item.
     *
     * @return The data associated with the item.
     */
    [[nodiscard]] const json::json &get_data() const { return data; }

  private:
    const std::string id;   // The ID of the item.
    const type &tp;         // The type of the item.
    const std::string name; // The name of the item.
    json::json data;        // The data associated with the item.
  };

  /**
   * Converts an item object to a JSON object.
   *
   * @param s The item object to convert.
   * @return A JSON object representing the item.
   */
  inline json::json to_json(const item &s) { return {{"id", s.get_id()}, {"type", s.get_type().get_id()}, {"name", s.get_name()}, {"data", s.get_data()}}; }

  const json::json coco_item_schema{"coco_item",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {"type", "integer"}},
                                       {"type", {"type", "integer"}},
                                       {"name", {"type", "string"}},
                                       {"data", {"type", "object"}}}}}};
} // namespace coco
