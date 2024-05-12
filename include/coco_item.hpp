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
     * @param static_pars The static parameters of the item.
     */
    item(const std::string &id, const type &tp, const std::string &name, std::vector<std::unique_ptr<parameter>> &&static_pars);

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
     * @brief Gets the static parameters of the item.
     *
     * @return The static parameters of the item.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<parameter>> &get_parameters() const { return parameters; }

  private:
    const std::string id;                               // The ID of the item.
    const type &tp;                                     // The type of the item.
    const std::string name;                             // The name of the item.
    std::vector<std::unique_ptr<parameter>> parameters; // The parameters of the item.
  };

  /**
   * Converts an item object to a JSON object.
   *
   * @param s The item object to convert.
   * @return A JSON object representing the item.
   */
  inline json::json to_json(const item &s)
  {
    json::json j{{"id", s.get_id()}, {"type", s.get_type().get_id()}, {"name", s.get_name()}};
    if (!s.get_parameters().empty())
    {
      json::json parameters;
      for (const auto &p : s.get_parameters())
        parameters.push_back(to_json(*p));
      j["parameters"] = parameters;
    }
    return j;
  }

  const json::json coco_item_schema{"coco_item",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {"type", "integer"}},
                                       {"type", {"type", "integer"}},
                                       {"name", {"type", "string"}},
                                       {"parameters", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/parameter"}}}}}}}}};
} // namespace coco
