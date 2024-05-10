#pragma once

#include "coco_item.hpp"
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
     * @param pars The parameters of the type.
     * @return A reference to the created type.
     */
    type &create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars);

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
     * @param data Optional data for the item (default is an empty JSON object).
     * @return A reference to the created item.
     */
    item &create_item(const type &type, const std::string &name, json::json &&data = {});

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

  private:
    virtual void new_type(const type &s);
    virtual void updated_type(const type &s);
    virtual void deleted_type(const std::string &id);

    virtual void new_item(const item &s);
    virtual void updated_item(const item &s);
    virtual void deleted_item(const std::string &id);

    virtual void new_item_data(const item &s, const std::chrono::system_clock::time_point &timestamp, const json::json &data);
    virtual void new_item_state(const item &s, const std::chrono::system_clock::time_point &timestamp, const json::json &state);

    virtual void new_solver(const coco_executor &exec);
    virtual void deleted_solver(const uintptr_t id);

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
    std::unique_ptr<coco_db> db;
    std::set<std::unique_ptr<coco_executor>> executors;

  protected:
    Environment *env;         // the CLIPS environment..
    std::recursive_mutex mtx; // mutex for the core..
  };
} // namespace coco
