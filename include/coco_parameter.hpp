#pragma once

#include "json.hpp"
#include <functional>

namespace coco
{
  class coco_db;
  class type;

  /**
   * @brief Represents a parameter object.
   *
   * This class encapsulates the information of a parameter, including its ID, name, and schema.
   */
  class parameter
  {
    friend class coco_db;

  public:
    /**
     * @brief Constructs a parameter object.
     *
     * @param id The ID of the parameter.
     * @param name The name of the parameter.
     * @param description The description of the parameter.
     * @param schema The schema of the parameter.
     */
    parameter(const std::string &id, const std::string &name, const std::string &description, json::json &&schema) : id(id), name(name), description(description), schema(std::move(schema)) {}

    /**
     * @brief Gets the ID of the parameter.
     *
     * @return The ID of the parameter.
     */
    const std::string &get_id() const { return id; }

    /**
     * @brief Gets the name of the parameter.
     *
     * @return The name of the parameter.
     */
    const std::string &get_name() const { return name; }

    /**
     * @brief Gets the description of the parameter.
     *
     * @return The description of the parameter.
     */
    const std::string &get_description() const { return description; }

    /**
     * @brief Gets the super parameters of the parameter.
     *
     * @return The super parameters of the parameter.
     */
    const std::vector<std::reference_wrapper<const parameter>> &get_super_parameters() const { return super_parameters; }

    /**
     * @brief Gets the disjoint parameters of the parameter.
     *
     * @return The disjoint parameters of the parameter.
     */
    const std::vector<std::reference_wrapper<const parameter>> &get_disjoint_parameters() const { return disjoint_parameters; }

    /**
     * @brief Gets the schema of the parameter.
     *
     * @return The schema of the parameter.
     */
    const json::json &get_schema() const { return schema; }

  private:
    std::string id;                                                           // The ID of the parameter.
    std::string name;                                                         // The name of the parameter.
    std::string description;                                                  // The description of the parameter.
    std::vector<std::reference_wrapper<const parameter>> super_parameters;    // The super parameters of the parameter.
    std::vector<std::reference_wrapper<const parameter>> disjoint_parameters; // The disjoint parameters of the parameter.
    json::json schema;                                                        // The schema of the parameter.
  };

  /**
   * Converts a parameter object to a JSON object.
   *
   * @param par The parameter object to convert.
   * @return The JSON object representing the parameter.
   */
  inline json::json to_json(const parameter &par)
  {
    json::json j{{"id", par.get_id()}, {"name", par.get_name()}, {"description", par.get_description()}, {"schema", par.get_schema()}};
    if (!par.get_super_parameters().empty())
    {
      json::json j_super_parameters(json::json_type::array);
      for (const auto &sp : par.get_super_parameters())
        j_super_parameters.push_back(sp.get().get_id());
      j["super_parameters"] = j_super_parameters;
    }
    if (!par.get_disjoint_parameters().empty())
    {
      json::json j_disjoint_parameters(json::json_type::array);
      for (const auto &dp : par.get_disjoint_parameters())
        j_disjoint_parameters.push_back(dp.get().get_id());
      j["disjoint_parameters"] = j_disjoint_parameters;
    }
    return j;
  }

  const json::json coco_parameter_schema = {"coco_parameter",
                                            {{"type", "object"},
                                             {"properties",
                                              {{"id", {{"type", "string"}}},
                                               {"name", {{"type", "string"}}},
                                               {"description", {{"type", "string"}}},
                                               {"super_parameters", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                               {"disjoint_parameters", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
                                               {"schema", {{"type", "object"}}}},
                                              {"required", {"id", "name", "schema"}}}}};
  const json::json parameters_path{"/parameters",
                                   {{"get",
                                     {{"summary", "Retrieve all the CoCo parameters"},
                                      {"description", "Endpoint to fetch all the managed parameters"},
                                      {"responses",
                                       {{"200",
                                         {{"description", "Successful response with the stored parameters"},
                                          {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}}}}}}}},
                                    {"post",
                                     {{"summary", "Create a new parameter"},
                                      {"description", "Endpoint to create a new parameter"},
                                      {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}},
                                      {"responses",
                                       {{"200",
                                         {{"description", "Successful response"}}}}}}}}};
  const json::json parameters_id_path{"/parameters/{parameter_id}",
                                      {{"get",
                                        {{"summary", "Retrieve the given parameter"},
                                         {"description", "Endpoint to fetch the given parameter"},
                                         {"parameters",
                                          {{{"name", "parameter_id"},
                                            {"in", "path"},
                                            {"required", true},
                                            {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                         {"responses",
                                          {{"200",
                                            {{"description", "Successful response with the stored parameter"},
                                             {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}}}}}},
                                       {"put",
                                        {{"summary", "Update the given parameter"},
                                         {"description", "Endpoint to update the given parameter"},
                                         {"parameters",
                                          {{{"name", "parameter_id"},
                                            {"in", "path"},
                                            {"required", true},
                                            {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                         {"requestBody", {{"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}},
                                         {"responses",
                                          {{"200",
                                            {{"description", "Successful response"}}},
                                           {"404",
                                            {{"description", "Type not found"},
                                             {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
                                       {"delete",
                                        {{"summary", "Delete the given parameter"},
                                         {"description", "Endpoint to delete the given parameter"},
                                         {"parameters",
                                          {{{"name", "parameter_id"},
                                            {"in", "path"},
                                            {"required", true},
                                            {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                         {"responses",
                                          {{"204",
                                            {{"description", "Successful response"}}},
                                           {"404",
                                            {{"description", "Type not found"},
                                             {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json new_parameter_message{
      {"new_parameter_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"new_parameter"}}}},
           {"new_parameter", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}};
  const json::json updated_parameter_message{
      {"updated_parameter_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"updated_parameter"}}}},
           {"updated_parameter", {{"$ref", "#/components/schemas/coco_parameter"}}}}}}}}};
  const json::json deleted_parameter_message{
      {"deleted_parameter_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deleted_parameter"}}}},
           {"deleted_parameter", {{"type", "string"}, {"format", "uuid"}}}}}}}}};
} // namespace coco
