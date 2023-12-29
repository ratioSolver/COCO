#pragma once

#include "sensor_type.h"
#include "location.h"
#include <chrono>

namespace coco
{
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
    sensor(const std::string &id, const std::string &name, sensor_type &type, location_ptr l) : id(id), name(name), type(type), loc(std::move(l)) {}
    virtual ~sensor() = default;

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
     * @brief Get the last update of the sensor.
     *
     * @return const std::chrono::system_clock& the last update of the sensor.
     */
    const std::chrono::system_clock::time_point &get_last_update() const { return last_update; }
    /**
     * @brief Check whether the sensor has a value.
     *
     * @return true if the sensor has a value.
     * @return false if the sensor has no value.
     */
    bool has_value() const { return value.operator bool(); }
    /**
     * @brief Get the value of the sensor.
     *
     * @return const json::json& the value of the sensor.
     */
    const json::json &get_value() const { return value; }
    /**
     * @brief Check whether the sensor has a state.
     *
     * @return true if the sensor has a state.
     * @return false if the sensor has no state.
     */
    bool has_state() const { return state.operator bool(); }
    /**
     * @brief Get the state of the sensor.
     *
     * @return const json::json& the state of the sensor.
     */
    const json::json &get_state() const { return state; }
    /**
     * @brief Get the fact of the sensor.
     *
     * @return Fact* the fact of the sensor.
     */
    Fact *get_fact() const { return fact; }

  private:
    void set_location(location_ptr l) { loc.swap(l); }

    void set_value(const std::chrono::system_clock::time_point &time, const json::json &val)
    {
      last_update = time;
      value = val; // we copy the value..
    }

    void set_state(const std::chrono::system_clock::time_point &time, const json::json &st)
    {
      last_update = time;
      state = st; // we copy the state..
    }

  private:
    const std::string id;
    std::string name;
    sensor_type &type;
    location_ptr loc;
    std::chrono::system_clock::time_point last_update = std::chrono::system_clock::from_time_t(0);
    json::json value;
    json::json state;
    Fact *fact = nullptr;
  };

  using sensor_ptr = std::unique_ptr<sensor>;

  inline json::json to_json(const sensor &s)
  {
    json::json j_s{{"id", s.get_id()}, {"name", s.get_name()}, {"type", s.get_type().get_id()}};
    if (s.has_location())
      j_s["location"] = {{"y", s.get_location().y}, {"x", s.get_location().x}};
    if (s.has_value())
      j_s["value"] = json::load(s.get_value().to_string());
    if (s.has_state())
      j_s["state"] = json::load(s.get_state().to_string());
    if (s.has_value() || s.has_state())
      j_s["timestamp"] = std::chrono::system_clock::to_time_t(s.get_last_update());
    return j_s;
  }
} // namespace coco
