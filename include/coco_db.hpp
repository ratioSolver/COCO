#pragma once

#include "json.hpp"
#include <optional>

namespace coco
{
  struct db_type
  {
    std::string name;
    std::vector<std::string> parents;
    std::optional<json::json> data, static_props, dynamic_props;
  };

  class coco_db
  {
  public:
    coco_db(json::json &&cnfg = {}) noexcept;

    virtual void drop() noexcept = 0;

    [[nodiscard]] virtual std::vector<db_type> get_types() noexcept = 0;
    virtual void create_type(std::string_view name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props) = 0;
    virtual void set_parents(std::string_view name, const std::vector<std::string> &parents) = 0;

  protected:
    const json::json config;
  };
} // namespace coco
