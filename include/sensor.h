#pragma once

#include "json.h"
#include "clips.h"

namespace coco
{
  class coco_core;
  class coco_db;

  class sensor
  {
    friend class coco_core;
    friend class coco_db;

  public:
    /**
     * @brief Construct a new sensor object.
     *
     * @param id the id of the sensor.
     * @param name the name of the sensor.
     * @param type the type of the sensor.
     * @param l the location of the sensor.
     */
    sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l) : id(id), name(name), type(type), loc(std::move(l)) {}
    ~sensor() = default;

    /**
     * @brief Get the id of the sensor.
     *
     * @return const std::string& the id of the sensor.
     */
    const std::string &get_id() const { return id; }
    /**
     * @brief Get the name of the sensor.
     *
     * @return const std::string& the name of the sensor.
     */
    const std::string &get_name() const { return name; }
    /**
     * @brief Get the type of the sensor.
     *
     * @return const sensor_type& the type of the sensor.
     */
    const sensor_type &get_type() const { return type; }
    /**
     * @brief Check whether the sensor has a location.
     *
     * @return true if the sensor has a location.
     * @return false if the sensor has no location.
     */
    bool has_location() const { return loc.operator bool(); }
    /**
     * @brief Get the location of the sensor.
     *
     * @return const location& the location of the sensor.
     */
    const location &get_location() const { return *loc; }
    /**
     * @brief Get the value of the sensor.
     *
     * @return const json::json& the value of the sensor.
     */
    const json::json &get_value() const { return *value; }
    /**
     * @brief Get the fact of the sensor.
     *
     * @return Fact* the fact of the sensor.
     */
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
