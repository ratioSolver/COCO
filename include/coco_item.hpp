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
     * @param id The ID of the item.
     * @param tp The type of the item.
     * @param name The name of the item.
     * @param pars The parameters of the item.
     */
    item(const std::string &id, const type &tp, const std::string &name, const json::json &pars);

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
     * @brief Gets the parameters of the item.
     *
     * @return The parameters of the item.
     */
    [[nodiscard]] const json::json &get_parameters() const { return parameters; }

  private:
    const std::string id;  // The ID of the item.
    const type &tp;        // The type of the item.
    std::string name;      // The name of the item.
    json::json parameters; // The parameters of the item.
  };

  /**
   * Converts an item object to a JSON object.
   *
   * @param s The item object to convert.
   * @return A JSON object representing the item.
   */
  inline json::json to_json(const item &s) { return json::json{{"id", s.get_id()}, {"type", s.get_type().get_id()}, {"name", s.get_name()}, {"parameters", s.get_parameters()}}; }

  const json::json coco_item_schema{"coco_item",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {{"type", "string"}, {"format", "uuid"}}},
                                       {"type", {{"type", "string"}, {"format", "uuid"}}},
                                       {"name", {{"type", "string"}}},
                                       {"parameters", {{"type", "object"}}}}}}};
  const json::json items_path{"/items",
                              {{"get",
                                {{"summary", "Retrieve all the CoCo items"},
                                 {"description", "Endpoint to fetch all the managed items"},
                                 {"responses",
                                  {{"200",
                                    {{"description", "Successful response with the stored items"},
                                     {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}}}}}}}},
                               {"post",
                                {{"summary", "Create a new item"},
                                 {"description", "Endpoint to create a new item"},
                                 {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}},
                                 {"responses",
                                  {{"200",
                                    {{"description", "Successful response"}}},
                                   {"400",
                                    {{"description", "Invalid parameters"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
                                   {"404",
                                    {{"description", "Type not found"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json items_id_path{"/items/{item_id}",
                                 {{"get",
                                   {{"summary", "Retrieve the given item"},
                                    {"description", "Endpoint to fetch the given item"},
                                    {"parameters",
                                     {{{"name", "item_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"responses",
                                     {{"200",
                                       {{"description", "Successful response with the given item"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}},
                                      {"404",
                                       {{"description", "Item not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
                                  {"put",
                                   {{"summary", "Update the given item"},
                                    {"description", "Endpoint to update the given item"},
                                    {"parameters",
                                     {{{"name", "item_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}},
                                    {"responses",
                                     {{"200",
                                       {{"description", "Successful response"}}},
                                      {"404",
                                       {{"description", "Item not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
                                  {"delete",
                                   {{"summary", "Delete the given item"},
                                    {"description", "Endpoint to delete the given item"},
                                    {"parameters",
                                     {{{"name", "item_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"responses",
                                     {{"204",
                                       {{"description", "Successful response with the deleted item"}}},
                                      {"404",
                                       {{"description", "Item not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json items_message{
      {"items_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"items"}}}},
           {"items", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}};
} // namespace coco
