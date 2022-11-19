#pragma once

#include "json.h"
#include "clips.h"

namespace coco
{
  class coco;
  class coco_db;

  class sensor
  {
    friend class coco;
    friend class coco_db;

  public:
    sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l) : id(id), name(name), type(type), loc(std::move(l)) {}
    ~sensor() = default;

    const std::string &get_id() const { return id; }
    const std::string &get_name() const { return name; }
    const sensor_type &get_type() const { return type; }
    const location &get_location() const { return *loc; }
    const json::json &get_value() const { return *value; }
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    std::string name;
    const sensor_type &type;
    std::unique_ptr<location> loc;
    std::unique_ptr<json::json> value;
    Fact *fact = nullptr;
  };
} // namespace coco
