#pragma once

#include "json.hpp"
#include "clips.h"
#include <memory>
#include <limits>

namespace coco
{
  class sensor_type
  {
  public:
    sensor_type();
    virtual ~sensor_type() = default;
  };
} // namespace coco
