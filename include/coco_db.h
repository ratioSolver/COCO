#pragma once

#include "sensor_type.h"
#include "sensor.h"
#include <unordered_map>

namespace coco
{
  class coco_core;
  class coco_db;

  class coco_db
  {
  public:
    coco_db(const std::string &root = COCO_ROOT);
    virtual ~coco_db() = default;

    const std::string &get_root() const { return root; }

    virtual void init() {}

    /**
     * @brief Create a sensor type object with the given name and description and returns its id.
     *
     * @param name the name of the sensor type
     * @param description the description of the sensor type
     * @param parameter_types the parameter types of the sensors of the sensor type
     * @return std::string the id of the created sensor type
     */
    virtual std::string create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types);
    /**
     * @brief Get the all sensor types object.
     *
     * @return std::vector<std::reference_wrapper<sensor_type>>
     */
    std::vector<std::reference_wrapper<sensor_type>> get_all_sensor_types();
    /**
     * @brief Get the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     * @return sensor_type& the sensor type with the given id.
     */
    sensor_type &get_sensor_type(const std::string &id) { return *sensor_types.at(id); }
    /**
     * @brief Set the name to the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     * @param name the new name of the sensor type.
     */
    virtual void set_sensor_type_name(const std::string &id, const std::string &name);
    /**
     * @brief Set the description to the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     * @param description the new description of the sensor type.
     */
    virtual void set_sensor_type_description(const std::string &id, const std::string &description);
    /**
     * @brief Delete the sensor type object with the given id.
     *
     * @param id the id of the sensor type.
     */
    virtual void delete_sensor_type(const std::string &id);

    /**
     * @brief Create a sensor object with the given name, type and location and returns its id.
     *
     * @param name the name of the sensor.
     * @param type the type of the sensor.
     * @param l the location of the sensor.
     * @return std::string the id of the created sensor.
     */
    virtual std::string create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l = nullptr);
    /**
     * @brief Get all the sensors object.
     *
     * @return std::vector<std::reference_wrapper<sensor>> all the sensors.
     */
    std::vector<std::reference_wrapper<sensor>> get_all_sensors();
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
     * @param id the id of the sensor.
     * @param name the new name of the sensor.
     */
    virtual void set_sensor_name(const std::string &id, const std::string &name);
    /**
     * @brief Set the type to the sensor object with the given id.
     *
     * @param id the id of the sensor.
     * @param type the new type of the sensor.
     */
    virtual void set_sensor_location(const std::string &id, std::unique_ptr<location> l);
    /**
     * @brief Set the value of the sensor object with the given id.
     *
     * @param id the id of the sensor.
     * @param time the time of the sensor value.
     * @param val the value of the sensor.
     */
    virtual void set_sensor_value(const std::string &id, const std::chrono::milliseconds::rep &time, const json::json &val);
    /**
     * @brief Delete the sensor object with the given id.
     *
     * @param id the id of the sensor.
     */
    virtual void delete_sensor(const std::string &id);

    /**
     * @brief Drop the database.
     *
     */
    virtual void drop();

  protected:
    void create_sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types);
    void create_sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l);

  private:
    const std::string root;
    std::unordered_map<std::string, std::unique_ptr<sensor_type>> sensor_types;
    std::unordered_map<std::string, std::unique_ptr<sensor>> sensors;
  };
} // namespace coco
