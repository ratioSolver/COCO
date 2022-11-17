#pragma once

#include "json.h"
#include "clips.h"
#include "timer.h"
#include "mqtt/async_client.h"
#include <mongocxx/client.hpp>

#define MQTT_URI(host, port) host ":" port
#define MONGODB_URI(host, port) "mongodb://" host ":" port

#define SOLVERS_TOPIC "/solvers"
#define SOLVER_TOPIC "/solver"
#define SENSORS_TOPIC "/sensors"
#define SENSOR_TOPIC "/sensor"

namespace coco
{
  class sensor_network;
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

  class mqtt_callback : public mqtt::callback
  {
  public:
    mqtt_callback(sensor_network &sn);

  private:
    void connected(const std::string &cause) override;
    void connection_lost(const std::string &cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

  private:
    sensor_network &sn;
  };

  class sensor_network
  {
    friend class mqtt_callback;
    friend class coco_executor;

  public:
    sensor_network(const std::string &root = COCO_ROOT, const std::string &mqtt_uri = MQTT_URI(MQTT_HOST, MQTT_PORT), const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    ~sensor_network();

    void connect();
    void disconnect();

  private:
    void tick();

    void update_sensor_network(json::json msg);

    friend void new_solver(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void read_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void read_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_script(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void adapt_files(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void delete_solver(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    const std::string root;
    mqtt::async_client mqtt_client;
    mqtt::connect_options options;
    mqtt_callback msg_callback;
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
