#pragma once

#include "coco_item.hpp"
#include "coco_rule.hpp"
#include "coco_executor.hpp"
#include <chrono>
#include <mutex>

namespace coco
{
  class coco_db;

  class coco_core
  {
    friend class coco_solver;
    friend class coco_executor;

  public:
    coco_core(std::unique_ptr<coco_db> &&db);
    virtual ~coco_core() = default;

    /**
     * @brief Returns a vector of references to the types.
     *
     * This function retrieves all the types stored in the database and returns them as a vector of `type` objects. The returned vector contains references to the actual types stored in the `types` map.
     *
     * @return A vector of types.
     */
    std::vector<std::reference_wrapper<type>> get_types();

    /**
     * @brief Creates a new type.
     *
     * This function creates a new type with the specified name, description, and parameters.
     *
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_pars The static parameters of the type.
     * @param dynamic_pars The dynamic parameters of the type.
     * @return A reference to the created type.
     */
    type &create_type(const std::string &name, const std::string &description, std::unordered_map<std::string, std::unique_ptr<parameter>> &&static_pars, std::unordered_map<std::string, std::unique_ptr<parameter>> &&dynamic_pars);

    /**
     * Retrieves a vector of references to the items in the database.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of references to the items.
     */
    std::vector<std::reference_wrapper<item>> get_items();

    /**
     * @brief Creates an item of the specified type with the given name and optional data.
     *
     * @param type The type of the item.
     * @param name The name of the item.
     * @param pars The parameters of the item.
     * @return A reference to the created item.
     */
    item &create_item(const type &type, const std::string &name, const json::json &pars);

    /**
     * @brief Adds data to the item.
     *
     * This function adds the provided data to the specified item.
     *
     * @param item The item to which the data will be added.
     * @param data The data to be added.
     */
    void add_data(const item &item, const json::json &data);

    /**
     * @brief Retrieves a vector of references to the solvers.
     *
     * This function retrieves all the solvers as a vector of `coco_executor` objects.
     *
     * @return A vector of references to the solvers.
     */
    std::vector<std::reference_wrapper<coco_executor>> get_solvers();

    /**
     * @brief Creates a solver with the specified name and units per tick.
     *
     * This function creates a solver with the specified name and units per tick.
     *
     * @param name The name of the solver.
     * @param units_per_tick The number of units per tick. Defaults to `utils::rational::one`.
     * @return A reference to the created `coco_executor` object.
     */
    coco_executor &create_solver(const std::string &name, const utils::rational &units_per_tick = utils::rational::one);

    /**
     * @brief Retrieves a vector of references to the reactive rules.
     *
     * This function retrieves all the reactive rules as a vector of `rule` objects.
     *
     * @return A vector of references to the reactive rules.
     */
    std::vector<std::reference_wrapper<rule>> get_reactive_rules();

    /**
     * @brief Creates a reactive rule with the specified name and content.
     *
     * This function creates a reactive rule with the specified name and content.
     *
     * @param name The name of the rule.
     * @param content The content of the rule.
     * @return A reference to the created `rule` object.
     */
    rule &create_reactive_rule(const std::string &name, const std::string &content);

    /**
     * @brief Retrieves a vector of references to the deliberative rules.
     *
     * This function retrieves all the deliberative rules as a vector of `rule` objects.
     *
     * @return A vector of references to the deliberative rules.
     */
    std::vector<std::reference_wrapper<rule>> get_deliberative_rules();

    /**
     * @brief Creates a deliberative rule with the specified name and content.
     *
     * This function creates a deliberative rule with the specified name and content.
     *
     * @param name The name of the rule.
     * @param content The content of the rule.
     * @return A reference to the created `rule` object.
     */
    rule &create_deliberative_rule(const std::string &name, const std::string &content);

  private:
    virtual void new_type(const type &s);
    virtual void updated_type(const type &s);
    virtual void deleted_type(const std::string &id);

    virtual void new_item(const item &s);
    virtual void updated_item(const item &s);
    virtual void deleted_item(const std::string &id);

    virtual void new_data(const item &s, const std::chrono::system_clock::time_point &timestamp, const json::json &data);

    virtual void new_solver(const coco_executor &exec);
    virtual void deleted_solver(const uintptr_t id);

    virtual void new_reactive_rule(const rule &r);
    virtual void new_deliberative_rule(const rule &r);

    virtual void state_changed(const coco_executor &exec);

    virtual void flaw_created(const coco_executor &exec, const ratio::flaw &f);
    virtual void flaw_state_changed(const coco_executor &exec, const ratio::flaw &f);
    virtual void flaw_cost_changed(const coco_executor &exec, const ratio::flaw &f);
    virtual void flaw_position_changed(const coco_executor &exec, const ratio::flaw &f);
    virtual void current_flaw(const coco_executor &exec, const ratio::flaw &f);

    virtual void resolver_created(const coco_executor &exec, const ratio::resolver &r);
    virtual void resolver_state_changed(const coco_executor &exec, const ratio::resolver &r);
    virtual void current_resolver(const coco_executor &exec, const ratio::resolver &r);

    virtual void causal_link_added(const coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r);

    virtual void executor_state_changed(const coco_executor &exec, ratio::executor::executor_state state);

    virtual void tick(const coco_executor &exec, const utils::rational &time);

    void starting(const coco_executor &exec, const std::vector<std::reference_wrapper<const ratio::atom>> &atoms);
    virtual void start(const coco_executor &exec, const std::vector<std::reference_wrapper<const ratio::atom>> &atoms);
    void ending(const coco_executor &exec, const std::vector<std::reference_wrapper<const ratio::atom>> &atoms);
    virtual void end(const coco_executor &exec, const std::vector<std::reference_wrapper<const ratio::atom>> &atoms);

  private:
    std::set<std::unique_ptr<coco_executor>> executors;

  protected:
    std::unique_ptr<coco_db> db; // the database..
    Environment *env;            // the CLIPS environment..
    std::recursive_mutex mtx;    // mutex for the core..
  };

  inline json::json make_types_message(coco_core &core)
  {
    json::json j;
    j["type"] = "types";
    json::json types = json::json_type::array;
    for (const auto &type : core.get_types())
      types.push_back(to_json(type));
    j["types"] = std::move(types);
    return j;
  }

  inline json::json make_items_message(coco_core &core)
  {
    json::json j;
    j["type"] = "items";
    json::json items = json::json_type::array;
    for (const auto &item : core.get_items())
      items.push_back(to_json(item));
    j["items"] = std::move(items);
    return j;
  }

  const json::json data_schema{"data",
                               {{"type", "object"},
                                {"properties",
                                 {{"timestamp", {{"type", "string"}, {"format", "date-time"}}},
                                  {"data", {{"type", "object"}}}}}}};
  const json::json error_schema{"error",
                                {{"type", "object"},
                                 {"properties",
                                  {{"code", {{"type", "integer"}}},
                                   {"message", {{"type", "string"}}}}}}};
  const json::json data_path{"/data/{item_id}",
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
                                    {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/error"}}}}}}}}}}}}}}};
} // namespace coco
