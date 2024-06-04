#pragma once

#include "parameter.hpp"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  /**
   * @brief Represents a CoCo type.
   *
   * This class provides information about a CoCo type, including its ID, name, description, and parameters.
   */
  class type final
  {
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
    type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars);

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
     * @brief Gets the static parameters of the CoCo type.
     *
     * @return The static parameters of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<parameter>> &get_static_parameters() const { return static_parameters; }

    /**
     * @brief Gets the dynamic parameters of the CoCo type.
     *
     * @return The dynamic parameters of the CoCo type.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<parameter>> &get_dynamic_parameters() const { return dynamic_parameters; }

  private:
    const std::string id;                                       // the ID of the CoCo type
    std::string name, description;                              // the name and description of the CoCo type
    std::vector<std::unique_ptr<parameter>> static_parameters;  // the static parameters of the CoCo type
    std::vector<std::unique_ptr<parameter>> dynamic_parameters; // the dynamic parameters of the CoCo type
  };

  /**
   * Converts a type object to a JSON representation.
   *
   * @param st The type object to convert.
   * @return The JSON representation of the type object.
   */
  inline json::json to_json(const type &st)
  {
    json::json j{{"id", st.get_id()}, {"name", st.get_name()}, {"description", st.get_description()}};
    json::json j_static_pars(json::json_type::array);
    for (const auto &p : st.get_static_parameters())
      j_static_pars.push_back(to_json(*p));
    j["static_parameters"] = std::move(j_static_pars);
    json::json j_dyn_pars(json::json_type::array);
    for (const auto &p : st.get_dynamic_parameters())
      j_dyn_pars.push_back(to_json(*p));
    j["dynamic_parameters"] = std::move(j_dyn_pars);
    return j;
  }

  const json::json coco_type_schema{"coco_type",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {{"type", "string"}, {"format", "uuid"}}},
                                       {"name", {{"type", "string"}}},
                                       {"description", {{"type", "string"}}},
                                       {"static_parameters", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/parameter"}}}}},
                                       {"dynamic_parameters", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/parameter"}}}}}}}}};
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
                                 {{"delete",
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
} // namespace coco
