#pragma once

#include "json.h"
#include "memory.h"
#include "clips.h"

namespace coco
{
  class coco_core;
  class coco_db;

  enum parameter_type
  {
    Integer,
    Float,
    Boolean,
    Symbol,
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
    const std::map<std::string, parameter_type> &get_parameters() const { return parameter_types; }
    /**
     * @brief Get the parameter type of the sensor type having the given name.
     *
     * @param name the name of the parameter.
     * @return const parameter_type& the parameter type of the sensor type having the given name.
     */
    const parameter_type &get_parameter_type(const std::string &name) const { return parameter_types.at(name); }
    /**
     * @brief Check if the sensor type has a parameter having the given name.
     *
     * @param name the name of the parameter.
     * @return true if the sensor type has a parameter having the given name.
     * @return false if the sensor type has no parameter having the given name.
     */
    bool has_parameter(const std::string &name) const { return parameter_types.find(name) != parameter_types.end(); }
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

  using sensor_type_ptr = utils::u_ptr<sensor_type>;
} // namespace coco
