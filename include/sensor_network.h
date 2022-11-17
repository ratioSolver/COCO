#pragma once

#include "json.h"
#include "clips.h"

#define BUILD_MQTT_URI(host, port) host ":" port
#define MQTT_URI(host, port) BUILD_MQTT_URI(host, port)
#define BUILD_MONGODB_URI(host, port) "mongodb://" host ":" port
#define MONGODB_URI(host, port) BUILD_MONGODB_URI(host, port)

namespace coco
{
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

  class sensor_network
  {
  public:
    sensor_network(const std::string &root = COCO_ROOT, const std::string &mqtt_uri = MQTT_URI(MQTT_HOST, MQTT_PORT), const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    ~sensor_network();

  private:
    const std::string root;
  };
} // namespace coco
