#pragma once

#include "coco_export.h"
#include "sensor_type.h"
#include "sensor.h"
#include "timer.h"
#include "solver.h"
#include "executor.h"
#include <unordered_set>
#include <list>
#include <mutex>

namespace coco
{
  class coco_core;
  class coco_db;
  class coco_executor;
  using coco_executor_ptr = std::unique_ptr<coco_executor>;
  class coco_listener;

  class coco_core
  {
    friend class coco_middleware;
    friend class coco_executor;
    friend class coco_listener;

  public:
    COCO_EXPORT coco_core(coco_db &db);
    COCO_EXPORT ~coco_core();

    /**
     * @brief Get the database object.
     *
     * @return coco_db& the database.
     */
    coco_db &get_database() { return db; }

    /**
     * @brief Get the mutex object. This mutex is used to synchronize the access to the system.
     *
     * @return std::recursive_mutex& the mutex.
     */
    std::recursive_mutex &get_mutex() { return mtx; }

    /**
     * @brief Get the list of executors.
     *
     * @return std::list<coco_executor_ptr>& the list of executors.
     */
    std::list<coco_executor_ptr> &get_executors() { return executors; }

    /**
     * @brief Loads the rules from the given files.
     *
     * @param files The files to load.
     */
    COCO_EXPORT void load_rules(const std::vector<std::string> &files);

    /**
     * @brief Starts the coco core.
     */
    void start() { coco_timer.start(); }
    /**
     * @brief Stops the coco core.
     */
    void stop() { coco_timer.stop(); }

    COCO_EXPORT void create_instance(const std::string &name, const json::json &data = {});

    COCO_EXPORT void create_sensor_type(const std::string &name, const std::string &description, std::vector<parameter_ptr> &&parameters);
    COCO_EXPORT void set_sensor_type_name(sensor_type &type, const std::string &name);
    COCO_EXPORT void set_sensor_type_description(sensor_type &type, const std::string &description);
    COCO_EXPORT void delete_sensor_type(sensor_type &type);

    COCO_EXPORT void create_sensor(const std::string &name, sensor_type &type, location_ptr l);
    COCO_EXPORT void set_sensor_name(sensor &s, const std::string &name);
    COCO_EXPORT void set_sensor_location(sensor &s, location_ptr l);
    COCO_EXPORT void set_sensor_data(sensor &s, const json::json &value);
    COCO_EXPORT void set_sensor_state(sensor &s, const json::json &state);
    COCO_EXPORT void delete_sensor(sensor &s);

  private:
    void tick();

    friend void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void new_solver_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void start_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void pause_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delay_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void extend_task(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void failure(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delete_solver(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void publish_message(Environment *env, UDFContext *udfc, UDFValue *out);

    static std::string value_to_string(const json::json &value)
    {
      std::string str;
      for (const auto &[id, val] : value.get_object())
        if (val.get_type() == json::json_type ::object)
          str += value_to_string(val);
        else
          str += ' ' + val.to_string();
      return str;
    }

  private:
    void fire_new_sensor_type(const sensor_type &type);
    void fire_updated_sensor_type(const sensor_type &type);
    void fire_removed_sensor_type(const std::string &id);

    void fire_new_sensor(const sensor &s);
    void fire_updated_sensor(const sensor &s);
    void fire_removed_sensor(const std::string &id);

    void fire_new_sensor_data(const sensor &s, const std::chrono::system_clock::time_point &time, const json::json &value);
    void fire_new_sensor_state(const sensor &s, const std::chrono::system_clock::time_point &time, const json::json &state);

    void fire_message_arrived(const std::string &topic, const json::json &msg);

    void fire_new_solver(const coco_executor &exec);
    void fire_removed_solver(const uintptr_t id);

    void fire_state_changed(const coco_executor &exec);

    void fire_started_solving(const coco_executor &exec);
    void fire_solution_found(const coco_executor &exec);
    void fire_inconsistent_problem(const coco_executor &exec);

    void fire_flaw_created(const coco_executor &exec, const ratio::flaw &f);
    void fire_flaw_state_changed(const coco_executor &exec, const ratio::flaw &f);
    void fire_flaw_cost_changed(const coco_executor &exec, const ratio::flaw &f);
    void fire_flaw_position_changed(const coco_executor &exec, const ratio::flaw &f);
    void fire_current_flaw(const coco_executor &exec, const ratio::flaw &f);

    void fire_resolver_created(const coco_executor &exec, const ratio::resolver &r);
    void fire_resolver_state_changed(const coco_executor &exec, const ratio::resolver &r);
    void fire_current_resolver(const coco_executor &exec, const ratio::resolver &r);

    void fire_causal_link_added(const coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r);

    void fire_executor_state_changed(const coco_executor &exec, ratio::executor::executor_state state);

    void fire_tick(const coco_executor &exec, const utils::rational &time);

    void fire_start(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms);
    void fire_end(const coco_executor &exec, const std::unordered_set<ratio::atom *> &atoms);

  private:
    coco_db &db;
    std::recursive_mutex mtx;
    ratio::time::timer coco_timer;
    std::list<coco_executor_ptr> executors;
    std::vector<coco_listener *> listeners; // the coco listeners..

  protected:
    Environment *env;
  };
} // namespace coco
