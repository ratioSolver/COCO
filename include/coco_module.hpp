#pragma once

#include "clips.h"
#include <mutex>
#include <string>

namespace coco
{
  class coco;

  class coco_module
  {
  public:
    coco_module(coco &cc) noexcept;
    virtual ~coco_module() = default;

  protected:
    [[nodiscard]] std::recursive_mutex &get_mtx() const;
    [[nodiscard]] Environment *get_env() const;

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  protected:
    coco &cc; // reference to the coco object
  };
} // namespace coco
