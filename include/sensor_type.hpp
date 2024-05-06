#pragma once

#include "parameter.hpp"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  /**
   * @brief Represents a sensor type.
   *
   * This class provides information about a sensor type, including its ID, name, description, and parameters.
   */
  class sensor_type final
  {
  public:
    /**
     * @brief Constructs a sensor type object.
     *
     * @param id The ID of the sensor type.
     * @param name The name of the sensor type.
     * @param description The description of the sensor type.
     * @param pars The parameters of the sensor type.
     */
    sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars);

    /**
     * @brief Gets the ID of the sensor type.
     *
     * @return The ID of the sensor type.
     */
    [[nodiscard]] const std::string &get_id() const { return id; }

    /**
     * @brief Gets the name of the sensor type.
     *
     * @return The name of the sensor type.
     */
    [[nodiscard]] const std::string &get_name() const { return name; }

    /**
     * @brief Gets the description of the sensor type.
     *
     * @return The description of the sensor type.
     */
    [[nodiscard]] const std::string &get_description() const { return description; }

    /**
     * @brief Gets the parameters of the sensor type.
     *
     * @return The parameters of the sensor type.
     */
    [[nodiscard]] const std::vector<std::unique_ptr<parameter>> &get_parameters() const { return parameters; }

  private:
    const std::string id;                               // the ID of the sensor type
    std::string name, description;                      // the name and description of the sensor type
    std::vector<std::unique_ptr<parameter>> parameters; // the parameters of the sensor type
  };

  /**
   * Converts a sensor_type object to a JSON representation.
   *
   * @param st The sensor_type object to convert.
   * @return The JSON representation of the sensor_type object.
   */
  inline json::json to_json(const sensor_type &st)
  {
    json::json j{{"id", st.get_id()}, {"name", st.get_name()}, {"description", st.get_description()}};
    json::json j_pars(json::json_type::array);
    for (const auto &p : st.get_parameters())
      j_pars.push_back(to_json(*p));
    j["parameters"] = std::move(j_pars);
    return j;
  }
} // namespace coco
