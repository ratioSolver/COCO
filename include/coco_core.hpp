#pragma once

#include "clips.h"

namespace coco
{
  class coco_db;

  class coco_core
  {
  public:
    coco_core(coco_db &db);
    virtual ~coco_core() = default;

  private:
    coco_db &db;

  protected:
    Environment *env;
  };
} // namespace coco
