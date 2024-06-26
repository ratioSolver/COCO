#pragma once

#include "sensor_type.h"
#include "sensor.h"
#include <unordered_map>
#include <functional>

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const std::string &name = COCO_NAME);
    virtual ~coco_db() = default;

    const std::string &get_name() const { return name; }
    const json::json &get_config() const { return config; }

    virtual void init();

    virtual std::string create_instance(const std::string &name = COCO_NAME, const json::json &data = {}) = 0;

    /**
     * @brief Create a sensor type object with the given name and description and returns its id.
     *
     * @param name the name of the sensor type
     * @param description the description of the sensor type
     * @param parameter_types the parameter types of the sensors of the sensor type
     * @return std::string the id of the created sensor type
     */
    virtual std::string create_sensor_type(const std::string &name, const std::string &description, std::vector<parameter_ptr> &&parameters);
    /**
     * @brief Get the all sensor types object.
     *
     * @return std::vector<std::reference_wrapper<sensor_type>>
     */
    std::vector<std::reference_wrapper<sensor_type>> get_sensor_types();
    /**
     * @brief Check if the sensor type object with the given id exists.
     *
     * @param id the id of the sensor type.
     * @return true if the sensor type exists.
     * @return false if the sensor type does not exist.
     */
    bool has_sensor_type(const std::string &id) const { return sensor_types.find(id) != sensor_types.end(); }
    /**
     * @brief Get the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     * @return sensor_type& the sensor type with the given id.
     */
    sensor_type &get_sensor_type(const std::string &id) { return *sensor_types.at(id); }
    /**
     * @brief Check if the sensor type object with the given name exists.
     *
     * @param name the name of the sensor type.
     * @return true if the sensor type exists.
     * @return false if the sensor type does not exist.
     */
    bool has_sensor_type_by_name(const std::string &name) const { return sensor_types_by_name.find(name) != sensor_types_by_name.end(); }
    /**
     * @brief Get the sensor type object with the given name.
     *
     * @param name the name of the sensor type.
     * @return sensor_type& the sensor type with the given name.
     */
    sensor_type &get_sensor_type_by_name(const std::string &name) { return sensor_types_by_name.at(name).get(); }
    /**
     * @brief Set the name to the sensor type object with the given id.
     *
     * @param st the sensor type.
     * @param name the new name of the sensor type.
     */
    virtual void set_sensor_type_name(sensor_type &st, const std::string &name) { st.name = name; }
    /**
     * @brief Set the description to the sensor type object with the given id.
     *
     * @param st the sensor type.
     * @param description the new description of the sensor type.
     */
    virtual void set_sensor_type_description(sensor_type &st, const std::string &description) { st.description = description; }
    /**
     * @brief Delete the sensor type object with the given id.
     *
     * @param st the sensor type.
     */
    virtual void delete_sensor_type(sensor_type &st) { sensor_types.erase(st.id); }

    /**
     * @brief Create a sensor object with the given name, type and location and returns its id.
     *
     * @param name the name of the sensor.
     * @param type the type of the sensor.
     * @param l the location of the sensor.
     * @return std::string the id of the created sensor.
     */
    virtual std::string create_sensor(const std::string &name, sensor_type &type, location_ptr l = nullptr);
    /**
     * @brief Get all the sensors object.
     *
     * @return std::vector<std::reference_wrapper<sensor>> all the sensors.
     */
    std::vector<std::reference_wrapper<sensor>> get_sensors();
    /**
     * @brief Check if the sensor object with the given id exists.
     *
     * @param id the id of the sensor.
     * @return true if the sensor exists.
     * @return false if the sensor does not exist.
     */
    bool has_sensor(const std::string &id) const { return sensors.find(id) != sensors.end(); }
    /**
     * @brief Get the sensor object with the given id.
     *
     * @param id the id of the sensor.
     * @return sensor& the sensor with the given id.
     */
    sensor &get_sensor(const std::string &id) { return *sensors.at(id); }
    /**
     * @brief Set the name to the sensor object with the given id.
     *
     * @param s the sensor.
     * @param name the new name of the sensor.
     */
    virtual void set_sensor_name(sensor &s, const std::string &name) { s.name = name; }
    /**
     * @brief Set the type to the sensor object with the given id.
     *
     * @param s the sensor.
     * @param type the new type of the sensor.
     */
    virtual void set_sensor_location(sensor &s, location_ptr l) { s.set_location(std::move(l)); }
    /**
     * @brief Get the last sensor value of the sensor object with the given id.
     *
     * @param s the sensor.
     * @return json::json the last sensor value of the sensor with the given id.
     */
    virtual json::json get_last_sensor_value(sensor &s);
    /**
     * @brief Get the sensor values of the sensor object from the given start time to the given end time.
     *
     * @param s the sensor.
     * @param from the start time of the sensor values.
     * @param to the end time of the sensor values.
     * @return json::json the sensor values of the sensor with the given id.
     */
    virtual json::json get_sensor_data(sensor &s, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to);
    /**
     * @brief Set the value of the sensor object with the given id.
     *
     * @param s the sensor.
     * @param time the time of the sensor value.
     * @param val the value of the sensor.
     */
    virtual void set_sensor_data(sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &val) { s.set_value(timestamp, val); }
    /**
     * @brief Delete the sensor object with the given id.
     *
     * @param s the sensor.
     */
    virtual void delete_sensor(sensor &s);

    /**
     * @brief Drop the database.
     *
     */
    virtual void drop();

  protected:
    void configure(const json::json &config) { this->config = config; }
    /**
     * @brief Create a sensor type object with the given id, name, description and parameter types and returns its id.
     *
     * @param id the id of the sensor type
     * @param name the name of the sensor type
     * @param description the description of the sensor type
     * @param parameter_types the parameter types of the sensors of the sensor type
     * @return std::string the id of the created sensor type
     */
    sensor_type &create_sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<parameter_ptr> &&parameters);
    /**
     * @brief Create a sensor object with the given id, name, type and location and returns its id.
     *
     * @param id the id of the sensor.
     * @param name the name of the sensor.
     * @param type the type of the sensor.
     * @param l the location of the sensor.
     * @return std::string the id of the created sensor.
     */
    sensor &create_sensor(const std::string &id, const std::string &name, sensor_type &type, location_ptr l);

  private:
    const std::string name;                                                                    // The app name.
    json::json config;                                                                         // The app config.
    std::unordered_map<std::string, sensor_type_ptr> sensor_types;                             // The sensor types of the app.
    std::unordered_map<std::string, std::reference_wrapper<sensor_type>> sensor_types_by_name; // The sensor types of the app indexed by name.
    std::unordered_map<std::string, sensor_ptr> sensors;                                       // The sensors of the app.
  };

  inline json::json sensor_types_message(const std::vector<std::reference_wrapper<sensor_type>> &rhs) noexcept
  {
    json::json j_msg{{"type", "sensor_types"}};
    json::json j_sts(json::json_type::array);
    for (const auto &st : rhs)
      j_sts.push_back(to_json(st.get()));
    j_msg["sensor_types"] = std::move(j_sts);
    return j_msg;
  }
  inline json::json new_sensor_type_message(const sensor_type &rhs) noexcept
  {
    json::json j_msg{{"type", "new_sensor_type"}};
    j_msg["sensor_type"] = to_json(rhs);
    return j_msg;
  }
  inline json::json updated_sensor_type_message(const sensor_type &rhs) noexcept
  {
    json::json j_msg{{"type", "updated_sensor_type"}};
    j_msg["sensor_type"] = to_json(rhs);
    return j_msg;
  }
  inline json::json deleted_sensor_type_message(const std::string &id) noexcept
  {
    json::json j_msg{{"type", "deleted_sensor_type"}};
    j_msg["sensor_type"] = id;
    return j_msg;
  }

  inline json::json sensors_message(const std::vector<std::reference_wrapper<sensor>> &rhs) noexcept
  {
    json::json j_msg{{"type", "sensors"}};
    json::json j_sensors(json::json_type::array);
    for (const auto &s : rhs)
      j_sensors.push_back(to_json(s.get()));
    j_msg["sensors"] = std::move(j_sensors);
    return j_msg;
  }
  inline json::json new_sensor_message(const sensor &rhs) noexcept
  {
    json::json j_msg{{"type", "new_sensor"}};
    j_msg["sensor"] = to_json(rhs);
    return j_msg;
  }
  inline json::json updated_sensor_message(const sensor &rhs) noexcept
  {
    json::json j_msg{{"type", "updated_sensor"}};
    j_msg["sensor"] = to_json(rhs);
    return j_msg;
  }
  inline json::json deleted_sensor_message(const std::string &id) noexcept
  {
    json::json j_msg{{"type", "deleted_sensor"}};
    j_msg["sensor"] = id;
    return j_msg;
  }
  inline json::json sensor_value_message(const coco::sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &value) noexcept
  {
    json::json j_msg{{"type", "sensor_value"}};
    j_msg["sensor"] = s.get_id();
    j_msg["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    j_msg["value"] = value;
    return j_msg;
  }
  inline json::json sensor_state_message(const coco::sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &state) noexcept
  {
    json::json j_msg{{"type", "sensor_state"}};
    j_msg["sensor"] = s.get_id();
    j_msg["timestamp"] = std::chrono::system_clock::to_time_t(timestamp);
    j_msg["state"] = state;
    return j_msg;
  }
} // namespace coco
