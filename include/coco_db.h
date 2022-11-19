#pragma once

#include "sensor_type.h"
#include "location.h"
#include "sensor.h"
#include <unordered_map>

namespace coco
{
  class coco;
  class coco_db;

  class coco_db
  {
  public:
    coco_db(const std::string &root = COCO_ROOT);
    virtual ~coco_db() = default;

    const std::string &get_root() const { return root; }

    virtual std::string create_sensor_type(const std::string &name, const std::string &description);
    sensor_type &get_sensor_type(const std::string &id) { return *sensor_types.at(id); }
    virtual void set_sensor_type_name(const std::string &id, const std::string &name);
    virtual void set_sensor_type_description(const std::string &id, const std::string &description);
    virtual void delete_sensor_type(const std::string &id);

    virtual std::string create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l);
    sensor &get_sensor(const std::string &id) { return *sensors.at(id); }
    virtual void set_sensor_name(const std::string &id, const std::string &name);
    virtual void set_sensor_type(const std::string &id, const sensor_type &type);
    virtual void set_sensor_location(const std::string &id, std::unique_ptr<location> l);
    virtual void set_sensor_value(const std::string &id, std::unique_ptr<json::json> v);
    virtual void delete_sensor(const std::string &id);

  protected:
    const std::string root;
    std::unordered_map<std::string, std::unique_ptr<sensor_type>> sensor_types;
    std::unordered_map<std::string, std::unique_ptr<sensor>> sensors;
  };
} // namespace coco
