#pragma once

#include "json.hpp"
#include <optional>
#include <chrono>

namespace coco
{
  struct db_type
  {
    std::string name;
    std::vector<std::string> parents;
    std::optional<json::json> data, static_props, dynamic_props;
  };

  struct db_item
  {
    std::string id, type;
    std::optional<json::json> props;
    std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value;
  };

  class coco_db
  {
  public:
    coco_db(json::json &&cnfg = {}) noexcept;

    virtual void drop() noexcept = 0;

    [[nodiscard]] virtual std::vector<db_type> get_types() noexcept = 0;
    virtual void create_type(std::string_view name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props) = 0;
    virtual void set_parents(std::string_view name, const std::vector<std::string> &parents) = 0;
    virtual void delete_type(std::string_view name) = 0;

    [[nodiscard]] virtual std::vector<db_item> get_items() noexcept = 0;
    virtual std::string create_item(std::string_view type, const json::json &props, const json::json &val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) = 0;

  protected:
    const json::json config;
  };
} // namespace coco
