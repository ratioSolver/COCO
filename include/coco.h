#pragma once

#include "json.h"
#include "clips.h"
#include "timer.h"
#include <list>

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

  class coco
  {
    friend class coco_middleware;
    friend class coco_executor;

  public:
    coco(coco_db &db);
    ~coco();

    void connect();
    void disconnect();

    coco_db &get_database() { return db; }

  private:
    void tick();

    void publish(const std::string &topic, const json::json &msg, int qos = 0, bool retained = false);
    void message_arrived(const json::json &msg);

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

  private:
    coco_db &db;
    std::list<std::unique_ptr<coco_middleware>> middlewares;
    ratio::time::timer coco_timer;
    std::list<std::unique_ptr<coco_executor>> executors;
    Environment *env;
  };
} // namespace coco
