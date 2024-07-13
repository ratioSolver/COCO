#pragma once

#include "coco_parameter.hpp"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  class coco_db;

  /**
   * @brief Represents a CoCo type.
   *
   * This class provides information about a CoCo type, including its ID, name, description, and parameters.
   */
  class type final
  {
    friend class coco_db;

  public:
    /**
     * @brief Constructs a CoCo type object.
     *
     * @param id The ID of the CoCo type.
     * @param name The name of the CoCo type.
     * @param description The description of the CoCo type.
     * @param static_pars The static parameters of the CoCo type.
     * @param dynamic_pars The dynamic parameters of the CoCo type.
     */
    type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const parameter>> &&static_pars, std::vector<std::reference_wrapper<const parameter>> &&dynamic_pars);

    /**
     * @brief Gets the ID of the CoCo type.
     *
     * @return The ID of the CoCo type.
     */
    [[nodiscard]] const std::string &get_id() const { return id; }

    /**
     * @brief Gets the name of the CoCo type.
     *
     * @return The name of the CoCo type.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the description of the CoCo type.
     *
     * @return The description of the CoCo type.
     */
    [[nodiscard]] const std::string &get_description() const { return description; }

    /**
     * @brief Gets the super types of the CoCo type.
     *
     * @return The super types of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::reference_wrapper<const type>> &get_super_types() const { return super_types; }

    /**
     * @brief Gets the disjoint types of the CoCo type.
     *
     * @return The disjoint types of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::reference_wrapper<const type>> &get_disjoint_types() const { return disjoint_types; }

    /**
     * @brief Gets the static parameters of the CoCo type.
     *
     * @return The static parameters of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::reference_wrapper<const parameter>> &get_static_parameters() const { return static_parameters; }

    /**
     * @brief Gets the dynamic parameters of the CoCo type.
     *
     * @return The dynamic parameters of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::reference_wrapper<const parameter>> &get_dynamic_parameters() const { return dynamic_parameters; }

  private:
    const std::string id;                                                    // the ID of the CoCo type
    std::string name, description;                                           // the name and description of the CoCo type
    std::vector<std::reference_wrapper<const type>> super_types;             // the super types of the CoCo type
    std::vector<std::reference_wrapper<const type>> disjoint_types;          // the disjoint types of the CoCo type
    std::vector<std::reference_wrapper<const parameter>> static_parameters;  // the static parameters of the CoCo type
    std::vector<std::reference_wrapper<const parameter>> dynamic_parameters; // the dynamic parameters of the CoCo type
  };

  /**
   * Converts a type object to a JSON representation.
   *
   * @param st The type object to convert.
   * @return The JSON representation of the type object.
   */
  inline json::json to_json(const type &t)
  {
    json::json j{{"id", t.get_id()}, {"name", t.get_name()}, {"description", t.get_description()}};
    if (!t.get_super_types().empty())
    {
      json::json j_super_types(json::json_type::array);
      for (const auto &st : t.get_super_types())
        j_super_types.push_back(st.get().get_id());
      j["super_types"] = std::move(j_super_types);
    }
    if (!t.get_disjoint_types().empty())
    {
      json::json j_disjoint_types(json::json_type::array);
      for (const auto &dt : t.get_disjoint_types())
        j_disjoint_types.push_back(dt.get().get_id());
      j["disjoint_types"] = std::move(j_disjoint_types);
    }
    if (!t.get_static_parameters().empty())
    {
      json::json j_static_parameters(json::json_type::array);
      for (const auto &sp : t.get_static_parameters())
        j_static_parameters.push_back(sp.get().get_id());
      j["static_parameters"] = std::move(j_static_parameters);
    }
    if (!t.get_dynamic_parameters().empty())
    {
      json::json j_dynamic_parameters(json::json_type::array);
      for (const auto &dp : t.get_dynamic_parameters())
        j_dynamic_parameters.push_back(dp.get().get_id());
      j["dynamic_parameters"] = std::move(j_dynamic_parameters);
    }
    return j;
  }

  const json::json coco_type_schema{"coco_type",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {{"type", "string"}, {"format", "uuid"}}},
                                       {"name", {{"type", "string"}}},
                                       {"description", {{"type", "string"}}},
                                       {"super_types", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                       {"disjoint_types", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                       {"static_parameters", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                       {"dynamic_parameters", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}}},
                                      {"required", std::vector<json::json>{"id", "name"}}}}};
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
                                    {{"description", "Successful response"}}}}}}}}};
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
                                       {{"description", "Successful response with the stored type"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}}}},
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
                                       {{"description", "Successful response"}}},
                                      {"404",
                                       {{"description", "Type not found"},
                                        {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
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
