#pragma once

#include "sensor_type.hpp"

namespace coco
{
  class sensor final
  {
  public:
    sensor(const std::string &id, const sensor_type &type, const std::string &name, json::json &&data = {});

    [[nodiscard]] const std::string &get_id() const { return id; }
    [[nodiscard]] const sensor_type &get_type() const { return type; }
    [[nodiscard]] const std::string &get_name() const { return name; }
    [[nodiscard]] const json::json &get_data() const { return data; }

  private:
    const std::string id;
    const sensor_type &type;
    const std::string name;
    json::json data;
  };
} // namespace coco
