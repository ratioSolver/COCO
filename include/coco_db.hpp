#pragma once

#include "sensor_type.hpp"
#include "sensor.hpp"

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const json::json &config);
    virtual ~coco_db() = default;

    const json::json &get_config() const { return config; }

    virtual sensor_type &create_sensor_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars) = 0;

  protected:
    sensor_type &create_sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars)
    {
      if (sensor_types.find(id) != sensor_types.end())
        throw std::invalid_argument("Sensor type already exists: " + id);
      sensor_types[id] = std::make_unique<sensor_type>(id, name, description, std::move(pars));
      return *sensor_types[id];
    }

  private:
    const json::json config; // The app name.

    std::map<std::string, std::unique_ptr<sensor_type>> sensor_types;
    std::map<std::string, std::unique_ptr<sensor>> sensors;
  };
} // namespace coco
