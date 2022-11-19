#pragma once

#include "json.h"
#include "clips.h"
#include "timer.h"
#include <list>
#include <mongocxx/client.hpp>

#define MONGODB_URI(host, port) "mongodb://" host ":" port

#define SOLVERS_TOPIC "/solvers"
#define SOLVER_TOPIC "/solver"
#define SENSORS_TOPIC "/sensors"
#define SENSOR_TOPIC "/sensor"

namespace coco
{
  class coco;
  class coco_middleware;
  class coco_executor;

  struct location
  {
    double x, y;
  };

  class sensor_type
  {
  public:
    sensor_type(const std::string &id, const std::string &name, const std::string &description, Fact *fact) : id(id), name(name), description(description), fact(fact) {}
    ~sensor_type() = default;

    const std::string &get_id() const { return id; }

    const std::string &get_name() const { return name; }
    void set_name(const std::string &n) { name = n; }

    const std::string &get_description() const { return description; }
    void set_description(const std::string &d) { description = d; }

    Fact *get_fact() const { return fact; }
    void set_fact(Fact *f) { fact = f; }

  private:
    const std::string id;
    std::string name;
    std::string description;
    Fact *fact;
  };

  class sensor
  {
  public:
    sensor(const std::string &id, const sensor_type &type, std::unique_ptr<location> l, Fact *fact) : id(id), type(type), loc(std::move(l)), fact(fact) {}
    ~sensor() = default;

    const std::string &get_id() const { return id; }
    const sensor_type &get_type() const { return type; }

    Fact *get_fact() const { return fact; }
    void set_fact(Fact *f) { fact = f; }

    location &get_location() const { return *loc; }
    void set_location(std::unique_ptr<location> l) { loc.swap(l); }

    json::json &get_value() const { return *value; }
    void set_value(std::unique_ptr<json::json> val) { value.swap(val); }

  private:
    const std::string id;
    const sensor_type &type;
    std::unique_ptr<location> loc;
    Fact *fact;
    std::unique_ptr<json::json> value;
  };

  class coco
  {
    friend class coco_middleware;
    friend class coco_executor;

  public:
    coco(const std::string &root = COCO_ROOT, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    ~coco();

    const std::string &get_root() const { return root; }

    void connect();
    void disconnect();

  private:
    void tick();

    void publish(const std::string &topic, const json::json &msg, int qos = 0, bool retained = false);
    void message_arrived(const json::json &msg);

    friend void new_solver_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void new_solver_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void start_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void pause_execution(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delete_solver(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    const std::string root;
    std::list<std::unique_ptr<coco_middleware>> middlewares;
    mongocxx::client conn;
    mongocxx::v_noabi::database db;
    mongocxx::v_noabi::collection sensor_types_collection;
    mongocxx::v_noabi::collection sensors_collection;
    mongocxx::v_noabi::collection sensor_data_collection;
    std::unordered_map<std::string, std::unique_ptr<sensor_type>> sensor_types;
    std::unordered_map<std::string, std::unique_ptr<sensor>> sensors;
    ratio::time::timer coco_timer;
    std::list<std::unique_ptr<coco_executor>> executors;
    Environment *env;
  };
} // namespace coco
