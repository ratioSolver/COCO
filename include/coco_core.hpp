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
