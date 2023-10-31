#pragma once

#include "json.h"
#include <memory>

namespace coco
{
  struct location
  {
    location(double x, double y) : x(x), y(y) {}

    double x, y;
  };

  using location_ptr = std::unique_ptr<location>;

  inline json::json to_json(const location &l) { return {{"x", l.x}, {"y", l.y}}; }
} // namespace coco
