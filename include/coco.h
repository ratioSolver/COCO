#pragma once

#include "coco_export.h"
#include "json.h"
#include "clips.h"
#include "timer.h"
#include "rational.h"
#include "item.h"
#include <unordered_set>
#include <list>
#include <mutex>

#define SOLVERS_TOPIC "/solvers"
#define SOLVER_TOPIC "/solver"
#define SENSORS_TOPIC "/sensors"
#define SENSOR_TOPIC "/sensor"

namespace coco
{
  class coco;
  class coco_middleware;
  class coco_db;
  class coco_executor;
  class coco_listener;

  class coco
  {
    friend class coco_middleware;
    friend class coco_executor;
    friend class coco_listener;

  public:
    COCO_EXPORT coco(coco_db &db);
    COCO_EXPORT ~coco();

    /**
     * @brief Get the database object.
     *
     * @return coco_db& the database.
     */
    coco_db &get_database() { return db; }

    /**
     * @brief Get the mutex object. This mutex is used to synchronize the access to the system.
     *
     * @return std::mutex& the mutex.
     */
    std::mutex &get_mutex() { return mtx; }

    /**
     * @brief Get the list of executors.
     *
     * @return std::list<std::unique_ptr<coco_executor>>& the list of executors.
     */
    std::list<std::unique_ptr<coco_executor>> &get_executors() { return executors; }

    /**
     * @brief Adds a middleware to the list of middlewares.
     *
     * @param mw The middleware to add.
     */
    void add_middleware(std::unique_ptr<coco_middleware> mw) { middlewares.push_back(std::move(mw)); }

    /**
     * @brief Loads the rules from the given files.
     *
     * @param files The files to load.
     */
    COCO_EXPORT void load_rules(const std::vector<std::string> &files);

    /**
     * @brief Connects all the middlewares and initializes the database.
     *
     */
    COCO_EXPORT void connect();
    /**
     * @brief Initializes the knowledge base.
     *
     */
    COCO_EXPORT void init();
    /**
     * @brief Disconnects all the middlewares.
     *
     */
    COCO_EXPORT void disconnect();

  private:
    void tick();

    void publish(const std::string &topic, json::json &msg, int qos = 0, bool retained = false);
    void message_arrived(const std::string &topic, json::json &msg);

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
    friend void send_message(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    void fire_new_solver(const coco_executor &exec);

    void fire_started_solving(const coco_executor &exec);
    void fire_solution_found(const coco_executor &exec);
    void fire_inconsistent_problem(const coco_executor &exec);

    void fire_message_arrived(const std::string &topic, json::json &msg);

    void fire_tick(const coco_executor &exec, const semitone::rational &time);

    void fire_start(const coco_executor &exec, const std::unordered_set<ratio::core::atom *> &atoms);
    void fire_end(const coco_executor &exec, const std::unordered_set<ratio::core::atom *> &atoms);

  private:
    coco_db &db;
    std::mutex mtx;
    std::list<std::unique_ptr<coco_middleware>> middlewares;
    ratio::time::timer coco_timer;
    std::list<std::unique_ptr<coco_executor>> executors;
    Environment *env;
    std::vector<coco_listener *> listeners; // the coco listeners..
  };
} // namespace coco
