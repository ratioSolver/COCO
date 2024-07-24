#pragma once

#include "coco_property.hpp"
#include <set>

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
     * @param cc The CoCo core object.
     * @param id The ID of the type.
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     */
    type(coco_core &cc, const std::string &id, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties) noexcept;
    ~type() noexcept;

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
     * @brief Gets the parent types of the type.
     *
     * @return The parent types of the type.
     */
    const std::map<std::string, std::reference_wrapper<const type>> &get_parents() const noexcept { return parents; }

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
    void add_parent(const type &parent) noexcept;
    void remove_parent(const type &parent) noexcept;
    void add_static_property(std::unique_ptr<property> &&prop) noexcept;
    void remove_static_property(const property &prop) noexcept;
    void add_dynamic_property(std::unique_ptr<property> &&prop) noexcept;
    void remove_dynamic_property(const property &prop) noexcept;

    /**
     * @brief Converts the `type` object to a JSON representation.
     *
     * @return The JSON representation of the `type` object.
     */
    json::json to_json() const noexcept;

  private:
    coco_core &cc;                                                       // The CoCo core object.
    Fact *type_fact = nullptr;                                           // The type fact.
    std::string id;                                                      // The ID of the type.
    std::string name;                                                    // The name of the type.
    std::string description;                                             // The description of the type.
    std::map<std::string, std::reference_wrapper<const type>> parents;   // The parent types of the type.
    std::map<std::string, Fact *> parent_facts;                          // The parent facts of the type.
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

  const json::json coco_type_schema{"coco_type",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {{"type", "string"}, {"format", "uuid"}}},
                                       {"name", {{"type", "string"}}},
                                       {"description", {{"type", "string"}}},
                                       {"parents", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                       {"static_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}}},
                                       {"dynamic_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}}}}}}};
  const json::json types_path{"/types",
                              {{"get",
                                {{"summary", "Retrieve all the CoCo types"},
                                 {"description", "Endpoint to fetch all the managed types"},
                                 {"responses",
                                  {{"200",
                                    {{"description", "Successful response with the stored types"},
                                     {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}}}}}},
                               {"post",
                                {{"summary", "Create a new type"},
                                 {"description", "Endpoint to create a new type"},
                                 {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
                                 {"responses",
                                  {{"200",
                                    {{"description", "Successful response"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
                                   {"400",
                                    {{"description", "Invalid parameters"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
                                   {"404",
                                    {{"description", "Parent type not found"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
                                   {"409",
                                    {{"description", "Type already exists"},
                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json types_id_path{"/types/{type_id}",
                                 {{"get",
                                   {{"summary", "Retrieve the given type"},
                                    {"description", "Endpoint to fetch the given type"},
                                    {"parameters",
                                     {{{"name", "type_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"responses",
                                     {{"200",
                                       {{"description", "Successful response with the given type"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
                                      {"404",
                                       {{"description", "Type not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
                                  {"put",
                                   {{"summary", "Update the given type"},
                                    {"description", "Endpoint to update the given type"},
                                    {"parameters",
                                     {{{"name", "type_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
                                    {"responses",
                                     {{"200",
                                       {{"description", "Successful response"}}},
                                      {"404",
                                       {{"description", "Type not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
                                  {"delete",
                                   {{"summary", "Delete the given type"},
                                    {"description", "Endpoint to delete the given type"},
                                    {"parameters",
                                     {{{"name", "type_id"},
                                       {"in", "path"},
                                       {"required", true},
                                       {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                    {"responses",
                                     {{"204",
                                       {{"description", "Successful response with the deleted type"}}},
                                      {"404",
                                       {{"description", "Type not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json types_message{
      {"types_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"types"}}}},
           {"types", {{"type", "array"}, {"types", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}};
  const json::json new_type_message{
      {"new_type_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"new_type"}}}},
           {"new_type", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}};
  const json::json updated_type_message{
      {"updated_type_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"updated_type"}}}},
           {"updated_type", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}};
  const json::json deleted_type_message{
      {"deleted_type_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deleted_type"}}}},
           {"deleted_type", {{"type", "string"}, {"format", "uuid"}}}}}}}}};
} // namespace coco
