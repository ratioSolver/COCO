#pragma once

#include "json.hpp"

namespace coco
{
  class coco_db
  {
  public:
    coco_db(json::json &&cnfg = {}) noexcept;

    virtual void drop() noexcept = 0;

    virtual void create_type(std::string_view name, const json::json &data, const json::json &static_props, const json::json &dynamic_props) = 0;

  protected:
    const json::json config;
  };
} // namespace coco
