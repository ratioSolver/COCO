#pragma once

#include "sensor.hpp"
#include "coco_executor.hpp"
#include <chrono>

namespace coco
{
  class coco_db;

  class coco_core
  {
    friend class coco_solver;

  public:
    coco_core(coco_db &db);
    virtual ~coco_core() = default;

    /**
     * @brief Creates a new sensor type.
     *
     * This function creates a new sensor type with the specified name, description, and parameters.
     *
     * @param name The name of the sensor type.
     * @param description The description of the sensor type.
     * @param pars The parameters of the sensor type.
     * @return A reference to the created sensor type.
     */
    sensor_type &create_sensor_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars);
    /**
     * @brief Creates a sensor of the specified type with the given name and optional data.
     *
     * @param type The type of the sensor.
     * @param name The name of the sensor.
     * @param data Optional data for the sensor (default is an empty JSON object).
     * @return A reference to the created sensor.
     */
    sensor &create_sensor(const sensor_type &type, const std::string &name, json::json &&data = {});

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
    virtual void new_sensor_type(const sensor_type &s);
    virtual void updated_sensor_type(const sensor_type &s);
    virtual void deleted_sensor_type(const std::string &id);

    virtual void new_sensor(const sensor &s);
    virtual void updated_sensor(const sensor &s);
    virtual void deleted_sensor(const std::string &id);

    virtual void new_sensor_value(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &value);
    virtual void new_sensor_state(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &state);

    virtual void new_solver(const coco_executor &exec);
    virtual void deleted_solver(const uintptr_t id);

    virtual void state_changed(const coco_executor &exec);

    virtual void started_solving(const coco_executor &exec);
    virtual void solution_found(const coco_executor &exec);
    virtual void inconsistent_problem(const coco_executor &exec);

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

    virtual void start(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms);
    virtual void end(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms);

  private:
    coco_db &db;
    std::set<std::unique_ptr<coco_executor>> executors;

  protected:
    Environment *env;
  };
} // namespace coco
