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
    sensor_type(const std::string &id);
    virtual ~sensor_type() = default;

    const std::string &get_id() const { return id; }

  private:
    const std::string id;
  };
} // namespace coco
