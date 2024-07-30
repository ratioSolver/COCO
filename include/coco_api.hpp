#pragma once

#include "executor_api.hpp"
#include <chrono>

namespace coco
{
  class coco_core;
  class property;
  class type;
  class item;
  class rule;

  /**
   * Converts a property object to a JSON representation.
   *
   * @param p The property object to convert.
   * @return The JSON representation of the property.
   */
  [[nodiscard]] json::json to_json(const property &p) noexcept;

  /**
   * Converts a type object to a JSON representation.
   *
   * @param t The type object to convert.
   * @return The JSON representation of the type object.
   */
  [[nodiscard]] json::json to_json(const type &t) noexcept;

  /**
   * Converts an item object to a JSON object.
   *
   * @param s The item object to convert.
   * @return A JSON object representing the item.
   */
  [[nodiscard]] json::json to_json(const item &s) noexcept;

  /**
   * @brief Converts a rule object to JSON format.
   *
   * @param r The rule object to convert.
   * @return The JSON representation of the rule.
   */
  [[nodiscard]] json::json to_json(const rule &r) noexcept;

  /**
   * @brief Creates a JSON message representing the taxonomy.
   *
   * This function takes a reference to a `coco_core` object and returns a JSON message
   * representing the taxonomy. The returned JSON message can be used for various purposes,
   * such as sending it over a network or storing it in a file.
   *
   * @param core The `coco_core` object containing the taxonomy information.
   * @return A JSON message representing the taxonomy.
   */
  [[nodiscard]] json::json make_taxonomy_message(coco_core &core) noexcept;

  /**
   * @brief Creates a new JSON message for the given type.
   *
   * This function takes a `type` object as input and returns a JSON message representing the type.
   *
   * @param tp The type object for which the JSON message is to be created.
   * @return A JSON message representing the given type.
   */
  [[nodiscard]] json::json make_new_type_message(const type &tp) noexcept;

  /**
   * @brief Creates an updated type message.
   *
   * This function takes a `type` object as input and returns a JSON object representing an updated type message.
   *
   * @param tp The `type` object to be used for creating the updated type message.
   * @return A JSON object representing the updated type message.
   */
  [[nodiscard]] json::json make_updated_type_message(const type &tp) noexcept;

  /**
   * @brief Creates a JSON message indicating a deleted type.
   *
   * This function creates a JSON message indicating that a type with the given ID has been deleted.
   * The message is returned as a `json::json` object.
   *
   * @param tp_id The ID of the deleted type.
   * @return A `json::json` object representing the deleted type message.
   */
  [[nodiscard]] json::json make_deleted_type_message(const std::string &tp_id) noexcept;

  /**
   * @brief Creates a JSON message containing items using the given `coco_core` object.
   *
   * This function creates a JSON message that contains items by utilizing the provided `coco_core` object.
   * The created JSON message can be used for further processing or communication.
   *
   * @param core The `coco_core` object used to generate the items message.
   * @return A JSON message containing items.
   */
  [[nodiscard]] json::json make_items_message(coco_core &core) noexcept;

  /**
   * @brief Creates a new item message.
   *
   * This function takes an item object and creates a JSON message representing the item.
   * The created JSON message is returned as a `json::json` object.
   *
   * @param itm The item object to create the message from.
   * @return The created JSON message as a `json::json` object.
   */
  [[nodiscard]] json::json make_new_item_message(const item &itm) noexcept;

  /**
   * @brief Creates a JSON message containing the updated item information.
   *
   * This function takes an `item` object and returns a JSON message that contains the updated information of the item.
   * The returned JSON message can be used for communication or storage purposes.
   *
   * @param itm The `item` object to be included in the message.
   * @return A JSON message containing the updated item information.
   */
  [[nodiscard]] json::json make_updated_item_message(const item &itm) noexcept;

  /**
   * @brief Creates a JSON message indicating that an item has been deleted.
   *
   * This function takes the ID of the deleted item as input and returns a JSON message
   * indicating that the item has been deleted.
   *
   * @param itm_id The ID of the deleted item.
   * @return A JSON message indicating that the item has been deleted.
   */
  [[nodiscard]] json::json make_deleted_item_message(const std::string &itm_id) noexcept;

  /**
   * @brief Creates a new data message.
   *
   * This function creates a new data message using the provided item, timestamp, and data.
   *
   * @param itm The item to include in the data message.
   * @param timestamp The timestamp of the data message.
   * @param data The data to include in the message.
   * @return A JSON object representing the new data message.
   */
  [[nodiscard]] json::json make_data_message(const item &itm, const std::chrono::system_clock::time_point &timestamp, const json::json &data) noexcept;

  /**
   * @brief Creates a JSON message containing reactive rules.
   *
   * This function takes a reference to a `coco_core` object and returns a JSON message
   * that contains the reactive rules. The message can be used for communication or
   * serialization purposes.
   *
   * @param core The `coco_core` object to retrieve the reactive rules from.
   * @return A JSON message containing the reactive rules.
   */
  [[nodiscard]] json::json make_reactive_rules_message(coco_core &core) noexcept;

  /**
   * @brief Creates a JSON message containing deliberative rules.
   *
   * This function takes a reference to a `coco_core` object and returns a JSON message
   * containing deliberative rules. The `nodiscard` attribute indicates that the return value
   * should not be ignored.
   *
   * @param core The `coco_core` object to retrieve the deliberative rules from.
   * @return A JSON message containing deliberative rules.
   */
  [[nodiscard]] json::json make_deliberative_rules_message(coco_core &core) noexcept;

  /**
   * @brief Creates a JSON message containing information about solvers.
   *
   * This function takes a reference to a `coco_core` object and creates a JSON message
   * containing information about the solvers. The message is returned as a `json::json` object.
   *
   * @param core The `coco_core` object representing the COCO platform.
   * @return A `json::json` object representing the solvers message.
   */
  [[nodiscard]] json::json make_solvers_message(coco_core &core) noexcept;

  /**
   * Creates a JSON message for a new reactive rule.
   *
   * This function takes a rule object and converts it into a JSON message.
   * It adds a "type" field to the JSON message indicating that it is a new reactive rule.
   *
   * @param r The rule object to convert into a JSON message.
   * @return The JSON message representing the new reactive rule.
   */
  [[nodiscard]] json::json make_reactive_rule_message(const rule &r) noexcept;

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
  [[nodiscard]] json::json make_deliberative_rule_message(const rule &r) noexcept;

  const json::json coco_schemas{
      {"property",
       {{"oneOf", std::vector<json::json>{{"$ref", "#/components/schemas/integer_property"}, {"$ref", "#/components/schemas/float_property"}, {"$ref", "#/components/schemas/string_property"}, {"$ref", "#/components/schemas/symbol_property"}, {"$ref", "#/components/schemas/item_property"}, {"$ref", "#/components/schemas/json_property"}}}}},
      {"integer_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"integer"}}}},
          {"description", {{"type", "string"}}},
          {"min", {{"type", "integer"}}},
          {"max", {{"type", "integer"}}}}},
        {"required", std::vector<json::json>{"type"}}}},
      {"float_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"float"}}}},
          {"description", {{"type", "string"}}},
          {"min", {{"type", "number"}}},
          {"max", {{"type", "number"}}}}},
        {"required", std::vector<json::json>{"type"}}}},
      {"string_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"string"}}}},
          {"description", {{"type", "string"}}}}},
        {"required", std::vector<json::json>{"type"}}}},
      {"symbol_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"symbol"}}}},
          {"description", {{"type", "string"}}},
          {"multiple", {{"type", "boolean"}}},
          {"values", {{"type", "array"}, {"items", {{"type", "string"}}}}},
          {"default", {{"type", "array"}, {"items", {{"type", "string"}}}}}}},
        {"required", std::vector<json::json>{"type"}}}},
      {"item_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"item"}}}},
          {"description", {{"type", "string"}}},
          {"type_id", {{"type", "string"}, {"format", "uuid"}}},
          {"multiple", {{"type", "boolean"}}},
          {"values", {{"type", "array"}, {"items", {{"type", "string"}}}}},
          {"default", {{"type", "array"}, {"items", {{"type", "string"}}}}}}},
        {"required", std::vector<json::json>{"type", "type_id"}}}},
      {"json_property",
       {{"type", "object"},
        {"properties",
         {{"type", {{"type", "string"}, {"enum", {"json"}}}},
          {"description", {{"type", "string"}}},
          {"schema", {{"type", "object"}}}}},
        {"required", std::vector<json::json>{"type", "schema"}}}},
      {"coco_type",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "string"}, {"format", "uuid"}}},
          {"name", {{"type", "string"}}},
          {"description", {{"type", "string"}}},
          {"parents", {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}},
          {"static_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}}},
          {"dynamic_properties", {{"type", "object"}, {"additionalProperties", {{"$ref", "#/components/schemas/property"}}}}}}}}},
      {"coco_item",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "string"}, {"format", "uuid"}}},
          {"type", {{"type", "string"}, {"format", "uuid"}}},
          {"name", {{"type", "string"}}},
          {"properties", {{"type", "object"}}}}}}},
      {"data",
       {{"type", "object"},
        {"properties",
         {{"timestamp", {{"type", "string"}, {"format", "date-time"}}},
          {"data", {{"type", "object"}}}}}}},
      {"coco_rule",
       {{"type", "object"},
        {"properties",
         {{"id", {{"type", "string"}, {"format", "uuid"}}},
          {"name", {{"type", "string"}}},
          {"content", {{"type", "string"}}}}},
        {"required", {"id", "name", "content"}}}},
      {"error",
       {{"type", "object"},
        {"properties",
         {{"message", {{"type", "string"}}}}}}}};

  const json::json coco_paths{
      {"/types",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/types/{type_id}",
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
             {{"description", "Successful response"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
            {"400",
             {{"description", "Invalid parameters"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
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
            {"400",
             {{"description", "Invalid parameters"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
            {"404",
             {{"description", "Type not found"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/items",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/items/{item_id}",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/data/{item_id}",
       {{"get",
         {{"summary", "Retrieve the data of the specified item"},
          {"description", "Endpoint to fetch the data of the specified item. The data can be filtered by specifying the start and end date"},
          {"parameters",
           {{{"name", "item_id"},
             {"description", "ID of the item"},
             {"in", "path"},
             {"required", true},
             {"schema", {{"type", "string"}, {"format", "uuid"}}}},
            {{"name", "from"},
             {"description", "Start date for filtering data"},
             {"in", "query"},
             {"schema", {{"type", "string"}, {"format", "date-time"}}}},
            {{"name", "to"},
             {"description", "End date for filtering data"},
             {"in", "query"},
             {"schema", {{"type", "string"}, {"format", "date-time"}}}}}},
          {"responses",
           {{"200",
             {{"description", "Successful response with the data of the specified item"},
              {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/data"}}}}}}}}}}},
            {"404",
             {{"description", "Item not found"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}},
        {"post",
         {{"summary", "Add data to the specified item"},
          {"description", "Endpoint to add data to the specified item"},
          {"parameters",
           {{{"name", "item_id"},
             {"description", "ID of the item"},
             {"in", "path"},
             {"required", true},
             {"schema", {{"type", "string"}, {"format", "uuid"}}}}}},
          {"requestBody", {{"content", {{"application/json", {{"schema", {{"type", "object"}}}}}}}}},
          {"responses",
           {{"200",
             {{"description", "Successful response"}}},
            {"400",
             {{"description", "Invalid input"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}},
            {"404",
             {{"description", "Item not found"},
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/reactive_rules",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}},
      {"/reactive_rules/{rule_id}",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}},
      {"/deliberative_rules",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}}}}}}},
      {"/deliberative_rules/{rule_id}",
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
              {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}}};

  const json::json coco_messages{
      {"taxonomy_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"taxonomy"}}}},
           {"types", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}},
      {"reactive_rules_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"reactive_rules"}}}},
           {"rules", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}},
      {"deliberative_rules_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deliberative_rules"}}}},
           {"rules", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_rule"}}}}}}}}}},
      {"solvers_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solvers"}}}},
           {"solvers", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/solver"}}}}}}}}}},
      {{"types_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"types"}}}},
            {"types", {{"type", "array"}, {"types", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}},
      {{"new_type_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"new_type"}}}},
            {"new_type", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
      {{"updated_type_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"updated_type"}}}},
            {"updated_type", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}},
      {"deleted_type_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deleted_type"}}}},
           {"deleted_type", {{"type", "string"}, {"format", "uuid"}}}}}}}},
      {{"items_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"items"}}}},
            {"items", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}}}},
      {{"new_item_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"new_item"}}}},
            {"new_item", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}},
      {{"updated_item_message",
        {"payload",
         {{"type", "object"},
          {"properties",
           {{"type", {{"type", "string"}, {"enum", {"updated_item"}}}},
            {"updated_item", {{"$ref", "#/components/schemas/coco_item"}}}}}}}}},
      {"deleted_item_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deleted_item"}}}},
           {"deleted_item", {{"type", "string"}, {"format", "uuid"}}}}}}}},
      {"data_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"data"}}}},
           {"item_id", {{"type", "string"}, {"format", "uuid"}}},
           {"timestamp", {{"type", "string"}, {"format", "date-time"}}},
           {"data", {{"type", "object"}}}}}}}}};
} // namespace coco
