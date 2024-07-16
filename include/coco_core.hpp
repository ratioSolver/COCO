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
     * @brief Retrieves a type with the specified ID.
     *
     * This function retrieves the type with the specified ID.
     *
     * @param id The ID of the type.
     * @return A reference to the type.
     * @throws std::invalid_argument if the type does not exist.
     */
    type &get_type(const std::string &id);

    /**
     * @brief Creates a new type.
     *
     * This function creates a new type with the specified name, description, and properties.
     *
     * @param name The name of the type.
     * @param description The description of the type.
     * @param static_props The static properties of the type.
     * @param dynamic_props The dynamic properties of the type.
     * @return A reference to the created type.
     */
    type &create_type(const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_props, std::map<std::string, std::unique_ptr<property>> &&dynamic_props);

    /**
     * @brief Deletes the type with the specified ID.
     *
     * This function deletes the type with the specified ID.
     *
     * @param id The ID of the type.
     * @throws std::invalid_argument if the type does not exist.
     */
    void delete_type(const std::string &id);

    /**
     * Retrieves a vector of references to the items in the database.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of references to the items.
     */
    std::vector<std::reference_wrapper<item>> get_items();

    /**
     * @brief Retrieves an item with the specified ID.
     *
     * This function retrieves the item with the specified ID.
     *
     * @param id The ID of the item.
     * @return A reference to the item.
     * @throws std::invalid_argument if the item does not exist.
     */
    item &get_item(const std::string &id);

    /**
     * @brief Creates an item of the specified type with the given name and optional data.
     *
     * @param tp The type of the item.
     * @param name The name of the item.
     * @param props The properties of the item.
     * @return A reference to the created item.
     */
    item &create_item(const type &tp, const std::string &name, const json::json &props);

    /**
     * @brief Deletes the item with the specified ID.
     *
     * This function deletes the item with the specified ID.
     *
     * @param id The ID of the item.
     * @throws std::invalid_argument if the item does not exist.
     */
    void delete_item(const std::string &id);

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
     * @brief Deletes the given solver.
     *
     * This function deletes the given solver.
     *
     * @param exec The solver to delete.
     */
    void delete_solver(coco_executor &exec);

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
    virtual void new_type(const type &tp);
    virtual void updated_type(const type &tp);
    virtual void deleted_type(const std::string &tp_id);

    virtual void new_item(const item &itm);
    virtual void updated_item(const item &itm);
    virtual void deleted_item(const std::string &itm_id);

    virtual void new_data(const item &itm, const std::chrono::system_clock::time_point &timestamp, const json::json &data);

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

    void reset_knowledge_base();

    friend void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void new_solver_rules(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void start_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void pause_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delay_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void extend_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void failure(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delete_solver(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    json::json schemas;                                 // the JSON schemas..
    std::set<std::unique_ptr<coco_executor>> executors; // the executors..

  protected:
    std::unique_ptr<coco_db> db; // the database..
    Environment *env;            // the CLIPS environment..
    std::recursive_mutex mtx;    // mutex for the core..
  };

  void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out);
  void new_solver_rules(Environment *env, UDFContext *udfc, UDFValue *out);
  void start_execution([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void pause_execution([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void delay_task([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void extend_task([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void failure([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void adapt_script([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void adapt_files([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);
  void delete_solver([[maybe_unused]] Environment *env, UDFContext *udfc, [[maybe_unused]] UDFValue *out);

  inline json::json make_taxonomy_message(coco_core &core)
  {
    json::json j;
    j["type"] = "taxonomy";
    json::json types = json::json_type::array;
    for (const auto &type : core.get_types())
      types.push_back(to_json(type));
    j["types"] = std::move(types);
    return j;
  }

  inline json::json make_new_type_message(const type &tp)
  {
    json::json j;
    j["type"] = "new_type";
    j["tp"] = to_json(tp);
    return j;
  }

  inline json::json make_updated_type_message(const type &tp)
  {
    json::json j;
    j["type"] = "updated_type";
    j["tp"] = to_json(tp);
    return j;
  }

  inline json::json make_deleted_type_message(const std::string &tp_id)
  {
    json::json j;
    j["type"] = "deleted_type";
    j["tp_id"] = tp_id;
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

  inline json::json make_new_item_message(const item &itm)
  {
    json::json j;
    j["type"] = "new_item";
    j["itm"] = to_json(itm);
    return j;
  }

  inline json::json make_updated_item_message(const item &itm)
  {
    json::json j;
    j["type"] = "updated_item";
    j["itm"] = to_json(itm);
    return j;
  }

  inline json::json make_deleted_item_message(const std::string &itm_id)
  {
    json::json j;
    j["type"] = "deleted_item";
    j["itm_id"] = itm_id;
    return j;
  }

  inline json::json make_reactive_rules_message(coco_core &core)
  {
    json::json j;
    j["type"] = "reactive_rules";
    json::json rules = json::json_type::array;
    for (const auto &r : core.get_reactive_rules())
      rules.push_back(to_json(r));
    j["rules"] = std::move(rules);
    return j;
  }

  inline json::json make_deliberative_rules_message(coco_core &core)
  {
    json::json j;
    j["type"] = "deliberative_rules";
    json::json rules = json::json_type::array;
    for (const auto &r : core.get_deliberative_rules())
      rules.push_back(to_json(r));
    j["rules"] = std::move(rules);
    return j;
  }

  inline json::json make_solvers_message(coco_core &core)
  {
    json::json j;
    j["type"] = "solvers";
    json::json solvers = json::json_type::array;
    for (const auto &exec : core.get_solvers())
      solvers.push_back(to_json(exec));
    j["solvers"] = std::move(solvers);
    return j;
  }

  const json::json taxonomy_message{
      {"types_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"taxonomy"}}}},
           {"properties", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_parameter"}}}}},
           {"types", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/coco_type"}}}}}}}}}}};
  const json::json reactive_rules_message{
      {"reactive_rules_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"reactive_rules"}}}},
           {"rules", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/rule"}}}}}}}}}}};
  const json::json deliberative_rules_message{
      {"deliberative_rules_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"deliberative_rules"}}}},
           {"rules", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/rule"}}}}}}}}}}};
  const json::json solvers_message{
      {"solvers_message",
       {"payload",
        {{"type", "object"},
         {"properties",
          {{"type", {{"type", "string"}, {"enum", {"solvers"}}}},
           {"solvers", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/solver"}}}}}}}}}}};
  const json::json error_schema{"error",
                                {{"type", "object"},
                                 {"properties",
                                  {{"code", {{"type", "integer"}}},
                                   {"message", {{"type", "string"}}}}}}};
} // namespace coco
