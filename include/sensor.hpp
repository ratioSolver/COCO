#pragma once

#include "sensor_type.hpp"

namespace coco
{
  class sensor
  {
  public:
    sensor(const std::string &id);
    virtual ~sensor() = default;

    [[nodiscard]] const std::string &get_id() const { return id; }

  private:
    const std::string id;
  };
} // namespace coco
