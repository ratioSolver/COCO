#pragma once

#include "sensor_type.hpp"
#include "sensor.hpp"

namespace coco
{
  class coco_db
  {
  public:
    coco_db(const json::json &config);
    virtual ~coco_db() = default;

    const json::json &get_config() const { return config; }

  private:
    const json::json config; // The app name.
  };
} // namespace coco
