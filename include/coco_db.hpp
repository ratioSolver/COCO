#pragma once

#include "sensor_type.hpp"
#include "sensor.hpp"

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const json::json &config = {});
    virtual ~coco_db() = default;

    const json::json &get_config() const { return config; }

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
    virtual sensor_type &create_sensor_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars) = 0;
    /**
     * @brief Creates a sensor of the specified type with the given name and optional data.
     *
     * @param type The type of the sensor.
     * @param name The name of the sensor.
     * @param data Optional data for the sensor (default is an empty JSON object).
     * @return A reference to the created sensor.
     */
    virtual sensor &create_sensor(const sensor_type &type, const std::string &name, json::json &&data = {}) = 0;

  protected:
    sensor_type &create_sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars)
    {
      if (sensor_types.find(id) != sensor_types.end())
        throw std::invalid_argument("Sensor type already exists: " + id);
      sensor_types[id] = std::make_unique<sensor_type>(id, name, description, std::move(pars));
      return *sensor_types[id];
    }
    sensor &create_sensor(const std::string &id, const sensor_type &type, const std::string &name, json::json &&data = {})
    {
      if (sensors.find(id) != sensors.end())
        throw std::invalid_argument("Sensor already exists: " + id);
      sensors[id] = std::make_unique<sensor>(id, type, name, std::move(data));
      return *sensors[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<sensor_type>> sensor_types;
    std::map<std::string, std::unique_ptr<sensor>> sensors;
  };
} // namespace coco
