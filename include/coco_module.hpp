#pragma once

#include "json.hpp"
#include "clips.h"
#include <mutex>
#include <string>

namespace coco
{
  class coco;

  class coco_module
  {
    friend class coco;

  public:
    coco_module(coco &cc) noexcept;
    virtual ~coco_module() = default;

  protected:
    [[nodiscard]] coco &get_coco() noexcept;

    [[nodiscard]] std::recursive_mutex &get_mtx() const;
    [[nodiscard]] Environment *get_env() const;

    [[nodiscard]] std::string to_string(Fact *f, std::size_t buff_size = 256) const noexcept;

  private:
    virtual void to_json(json::json &) const noexcept {}

  private:
    coco &cc; // reference to the coco object
  };
} // namespace coco
