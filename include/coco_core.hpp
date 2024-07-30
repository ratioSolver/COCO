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
    friend class type;
    friend class item;
    friend class property;
    friend class rule;

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
     * @param parents The parents of the type.
     * @param static_properties The static properties of the type.
     * @param dynamic_properties The dynamic properties of the type.
     * @return A reference to the created type.
     */
    type &create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties);

    /**
     * @brief Sets the name of the type.
     *
     * This function sets the name of the type with the specified ID.
     *
     * @param tp The type.
     * @param name The name of the type.
     */
    void set_type_name(type &tp, const std::string &name);

    /**
     * @brief Sets the description of the type.
     *
     * This function sets the description of the type with the specified ID.
     *
     * @param tp The type.
     * @param description The description of the type.
     */
    void set_type_description(type &tp, const std::string &description);

    /**
     * @brief Adds a parent to the type.
     *
     * This function adds a parent to the type with the specified ID.
     *
     * @param tp The type.
     * @param parent The parent to add.
     */
    void add_parent(type &tp, const type &parent);

    /**
     * @brief Removes a parent from the type.
     *
     * This function removes a parent from the type with the specified ID.
     *
     * @param tp The type.
     * @param parent The parent to remove.
     */
    void remove_parent(type &tp, const type &parent);

    /**
     * @brief Adds a static property to the type.
     *
     * This function adds a static property to the type with the specified ID.
     *
     * @param tp The type.
     * @param prop The property to add.
     */
    void add_static_property(type &tp, std::unique_ptr<property> &&prop);

    /**
     * @brief Removes a static property from the type.
     *
     * This function removes a static property from the type with the specified ID.
     *
     * @param tp The type.
     * @param prop The property to remove.
     */
    void remove_static_property(type &tp, const property &prop);

    /**
     * @brief Adds a dynamic property to the type.
     *
     * This function adds a dynamic property to the type with the specified ID.
     *
     * @param tp The type.
     * @param prop The property to add.
     */
    void add_dynamic_property(type &tp, std::unique_ptr<property> &&prop);

    /**
     * @brief Removes a dynamic property from the type.
     *
     * This function removes a dynamic property from the type with the specified ID.
     *
     * @param tp The type.
     * @param prop The property to remove.
     */
    void remove_dynamic_property(type &tp, const property &prop);

    /**
     * @brief Deletes the type with the specified ID.
     *
     * This function deletes the type with the specified ID.
     *
     * @param tp The type to delete.
     */
    void delete_type(const type &tp);

    /**
     * Retrieves a vector of references to the items in the database.
     *
     * This function retrieves all the items stored in the database and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @return A vector of references to the items.
     */
    std::vector<std::reference_wrapper<item>> get_items();

    /**
     * @brief Retrieves a vector of references to the items of the specified type.
     *
     * This function retrieves all the items of the specified type and returns them as a vector of `item` objects. The returned vector contains references to the actual items stored in the `items` map.
     *
     * @param tp The type of the items to retrieve.
     * @return A vector of references to the items.
     */
    std::vector<std::reference_wrapper<item>> get_items_by_type(const type &tp);

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
     * @brief Sets the name of the item.
     *
     * This function sets the name of the item with the specified ID.
     *
     * @param itm The item.
     * @param name The name of the item.
     */
    void set_item_name(item &itm, const std::string &name);

    /**
     * @brief Sets the properties of the item.
     *
     * This function sets the properties of the item with the specified ID.
     *
     * @param itm The item.
     * @param props The properties of the item.
     */
    void set_item_properties(item &itm, const json::json &props);

    /**
     * @brief Deletes the item with the specified ID.
     *
     * This function deletes the item with the specified ID.
     *
     * @param itm The item to delete.
     */
    void delete_item(const item &itm);

    /**
     * @brief Retrieves the data of the item.
     *
     * This function retrieves the data of the specified item within the specified time range.
     *
     * @param itm The item.
     * @param from The start of the time range.
     * @param to The end of the time range.
     * @return The data of the item.
     */
    json::json get_data(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to);

    /**
     * @brief Adds data to the item.
     *
     * This function adds the provided data to the specified item.
     *
     * @param item The item to which the data will be added.
     * @param data The data to be added.
     * @param timestamp The timestamp of the data. Defaults to the current time.
     */
    void add_data(const item &item, const json::json &data, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());

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

    virtual void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp);

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
} // namespace coco
