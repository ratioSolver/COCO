#pragma once

#include "memory.h"
#include "json.h"

namespace coco
{
  struct location
  {
    double x, y;
  };

  using location_ptr = utils::u_ptr<location>;

  inline json::json to_json(const location &l) { return {{"x", l.x}, {"y", l.y}}; }
} // namespace coco
