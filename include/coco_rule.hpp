#pragma once

#include "json.hpp"
#include <string>

namespace coco
{
  /**
   * @brief Represents a CoCo rule.
   *
   * This class represents a CoCo rule, either reactive or deliberative, in the form of a name and content.
   */
  class rule
  {
  public:
    /**
     * @brief Constructs a rule object.
     *
     * @param id The ID of the rule.
     * @param name The name of the rule.
     * @param content The content of the rule.
     */
    rule(const std::string &id, const std::string &name, const std::string &content) : id(id), name(name), content(content) {}

    /**
     * @brief Gets the ID of the rule.
     *
     * @return The ID of the rule.
     */
    const std::string &get_id() const { return id; }

    /**
     * @brief Gets the name of the rule.
     *
     * @return The name of the rule.
     */
    const std::string &get_name() const { return name; }

    /**
     * @brief Gets the content of the rule.
     *
     * @return The content of the rule.
     */
    const std::string &get_content() const { return content; }

  private:
    std::string id;      // the ID of the rule.
    std::string name;    // the name of the rule.
    std::string content; // the content of the rule.
  };

  /**
   * @brief Converts a rule object to JSON format.
   *
   * @param r The rule object to convert.
   * @return The JSON representation of the rule.
   */
  inline json::json to_json(const rule &r)
  {
    json::json j;
    j["id"] = r.get_id();
    j["name"] = r.get_name();
    j["content"] = r.get_content();
    return j;
  }

  /**
   * Creates a JSON message for a new reactive rule.
   *
   * This function takes a rule object and converts it into a JSON message.
   * It adds a "type" field to the JSON message indicating that it is a new reactive rule.
   *
   * @param r The rule object to convert into a JSON message.
   * @return The JSON message representing the new reactive rule.
   */
  inline json::json make_reactive_rule_message(const rule &r) noexcept
  {
    json::json j = to_json(r);
    j["type"] = "new_reactive_rule";
    return j;
  }

  /**
   * Creates a JSON message for a new deliberative rule.
   *
   * This function takes a rule object and converts it into a JSON message.
   * The resulting JSON message includes the rule data and a type field indicating
   * that it is a new deliberative rule.
   *
   * @param r The rule object to convert into a JSON message.
   * @return The JSON message representing the new deliberative rule.
   */
  inline json::json make_deliberative_rule_message(const rule &r) noexcept
  {
    json::json j = to_json(r);
    j["type"] = "new_deliberative_rule";
    return j;
  }

  const json::json coco_rule_schema{"coco_rule",
                                    {{"type", "object"},
                                     {"properties",
                                      {{"id", {{"type", "string"}, {"format", "uuid"}}},
                                       {"name", {{"type", "string"}}},
                                       {"content", {{"type", "string"}}}}}}};

  const json::json reactive_rules_path{"/reactive_rules",
                                       {{"get",
                                         {{"summary", "Retrieve all the CoCo reactive rules"},
                                          {"description", "Endpoint to fetch all the managed reactive rules"},
                                          {"responses",
                                           {{"200",
                                             {{"description", "Successful response with the stored reactive rules"},
                                              {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}},
                                        {"post",
                                         {{"summary", "Create a new CoCo reactive rule"},
                                          {"description", "Endpoint to create a new reactive rule"},
                                          {"requestBody",
                                           {{"content",
                                             {{"application/json",
                                               {{"schema", {"$ref", "#/components/schemas/coco_rule"}}}}}}}},
                                          {"responses",
                                           {{"201",
                                             {{"description", "Successful response with the created reactive rule"},
                                              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}};
  const json::json reactive_rules_id_path{"/reactive_rules/{rule_id}",
                                          {{"delete",
                                            {{"summary", "Delete the given CoCo reactive rule"},
                                             {"description", "Endpoint to delete the given reactive rule"},
                                             {"parameters",
                                              {{{"name", "rule_id"},
                                                {"in", "path"},
                                                {"required", true},
                                                {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                             {"responses",
                                              {{"204",
                                                {{"description", "Successful response with the deleted reactive rule"}}},
                                               {"404",
                                                {{"description", "Rule not found"},
                                                 {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
  const json::json deliberative_rules_path{"/deliberative_rules",
                                           {{"get",
                                             {{"summary", "Retrieve all the CoCo deliberative rules"},
                                              {"description", "Endpoint to fetch all the managed deliberative rules"},
                                              {"responses",
                                               {{"200",
                                                 {{"description", "Successful response with the stored deliberative rules"},
                                                  {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}},
                                            {"post",
                                             {{"summary", "Create a new CoCo deliberative rule"},
                                              {"description", "Endpoint to create a new deliberative rule"},
                                              {"requestBody",
                                               {{"content",
                                                 {{"application/json",
                                                   {{"schema", {"$ref", "#/components/schemas/coco_rule"}}}}}}}},
                                              {"responses",
                                               {{"201",
                                                 {{"description", "Successful response with the created deliberative rule"},
                                                  {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}};
  const json::json deliberative_rules_id_path{"/deliberative_rules/{rule_id}",
                                              {{"delete",
                                                {{"summary", "Delete the given CoCo deliberative rule"},
                                                 {"description", "Endpoint to delete the given deliberative rule"},
                                                 {"parameters",
                                                  {{{"name", "rule_id"},
                                                    {"in", "path"},
                                                    {"required", true},
                                                    {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
                                                 {"responses",
                                                  {{"204",
                                                    {{"description", "Successful response with the deleted deliberative rule"}}},
                                                   {"404",
                                                    {{"description", "Rule not found"},
                                                     {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
} // namespace coco
