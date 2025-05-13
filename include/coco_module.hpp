#pragma once

#include "clips.h"
#include <mutex>

namespace coco
{
  class coco;

  class coco_module
  {
  public:
    coco_module(coco &cc) noexcept : cc(cc) {}
    virtual ~coco_module() = default;

  protected:
    [[nodiscard]] std::recursive_mutex &get_mtx() const;
    [[nodiscard]] Environment *get_env() const;

  protected:
    coco &cc; // reference to the coco object
  };
} // namespace coco
