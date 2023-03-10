#pragma once

#include "memory.h"

namespace coco
{
  struct location
  {
    double x, y;
  };

  using location_ptr = utils::u_ptr<location>;
} // namespace coco
