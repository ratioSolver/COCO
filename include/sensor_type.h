#pragma once

#include "json.h"
#include "clips.h"

namespace coco
{
  class coco_core;
  class coco_db;

  enum parameter_type
  {
    Integer,
    Float,
    String
  };

  class sensor_type
  {
    friend class coco_core;
    friend class coco_db;

  public:
    /**
     * @brief Construct a new sensor type object.
     *
     * @param id the id of the sensor type.
     * @param name the name of the sensor type.
     * @param description the description of the sensor type.
     */
    sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) : id(id), name(name), description(description), parameter_types(parameter_types) {}
    ~sensor_type() = default;

    /**
     * @brief Get the id of the sensor type.
     *
     * @return const std::string& the id of the sensor type.
     */
    const std::string &get_id() const { return id; }
    /**
     * @brief Get the name of the sensor type.
     *
     * @return const std::string& the name of the sensor type.
     */
    const std::string &get_name() const { return name; }
    /**
     * @brief Get the description of the sensor type.
     *
     * @return const std::string& the description of the sensor type.
     */
    const std::string &get_description() const { return description; }
    /**
     * @brief Get the parameter types of the sensor type.
     *
     * @return const std::map<std::string, parameter_type>& the parameter types of the sensor type.
     */
    const std::map<std::string, parameter_type> &get_parameter_types() const { return parameter_types; }
    /**
     * @brief Get the fact of the sensor type.
     *
     * @return Fact* the fact of the sensor type.
     */
    Fact *get_fact() const { return fact; }

  private:
    const std::string id;
    std::string name;
    std::string description;
    std::map<std::string, parameter_type> parameter_types;
    Fact *fact = nullptr;
  };
} // namespace coco
