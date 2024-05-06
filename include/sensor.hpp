#pragma once

#include "sensor_type.hpp"

namespace coco
{
  /**
   * @brief Represents a sensor.
   *
   * This class provides functionality to create and manage sensors.
   */
  class sensor final
  {
  public:
    /**
     * @brief Constructs a sensor object.
     *
     * @param id The ID of the sensor.
     * @param type The type of the sensor.
     * @param name The name of the sensor.
     * @param data The data associated with the sensor (optional).
     */
    sensor(const std::string &id, const sensor_type &type, const std::string &name, json::json &&data = {});

    /**
     * @brief Gets the ID of the sensor.
     *
     * @return The ID of the sensor.
     */
    [[nodiscard]] const std::string &get_id() const { return id; }

    /**
     * @brief Gets the type of the sensor.
     *
     * @return The type of the sensor.
     */
    [[nodiscard]] const sensor_type &get_type() const { return type; }

    /**
     * @brief Gets the name of the sensor.
     *
     * @return The name of the sensor.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the data associated with the sensor.
     *
     * @return The data associated with the sensor.
     */
    [[nodiscard]] const json::json &get_data() const { return data; }

  private:
    const std::string id;    // The ID of the sensor.
    const sensor_type &type; // The type of the sensor.
    const std::string name;  // The name of the sensor.
    json::json data;         // The data associated with the sensor.
  };

  /**
   * Converts a sensor object to a JSON object.
   *
   * @param s The sensor object to convert.
   * @return A JSON object representing the sensor.
   */
  inline json::json to_json(const sensor &s) { return {{"id", s.get_id()}, {"type", s.get_type().get_id()}, {"name", s.get_name()}, {"data", s.get_data()}}; }
} // namespace coco
