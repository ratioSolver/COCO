#pragma once

#include "sensor.hpp"
#include "coco_executor.hpp"
#include <chrono>

namespace coco
{
  class coco_db;
  class coco_executor;

  class coco_core
  {
  public:
    coco_core(coco_db &db);
    virtual ~coco_core() = default;

  private:
    virtual void new_sensor_type(const sensor_type &s);
    virtual void updated_sensor_type(const sensor_type &s);
    virtual void deleted_sensor_type(const std::string &id);

    virtual void new_sensor(const sensor &s);
    virtual void updated_sensor(const sensor &s);
    virtual void deleted_sensor(const std::string &id);

    virtual void new_sensor_value(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &value);
    virtual void new_sensor_state(const sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &state);

  private:
    coco_db &db;
    std::set<std::unique_ptr<coco_executor>> executors;

  protected:
    Environment *env;
  };
} // namespace coco
