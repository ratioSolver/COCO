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

  struct db_rule
  {
    std::string name, content;
  };

  class coco_db
  {
  public:
    coco_db(json::json &&cnfg = {}) noexcept;

    virtual void drop() noexcept;

    [[nodiscard]] virtual std::vector<db_type> get_types() noexcept;
    virtual void create_type(std::string_view tp_name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props);
    virtual void set_parents(std::string_view tp_name, const std::vector<std::string> &parents);
    virtual void delete_type(std::string_view tp_name);

    [[nodiscard]] virtual std::vector<db_item> get_items() noexcept;
    virtual std::string create_item(std::string_view type, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val = std::nullopt);
    [[nodiscard]] virtual json::json get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to = std::chrono::system_clock::now());
    virtual void set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());
    virtual void delete_item(std::string_view itm_id);

    [[nodiscard]] virtual std::vector<db_rule> get_reactive_rules() noexcept;
    virtual void create_reactive_rule(std::string_view rule_name, std::string_view rule_content);
    [[nodiscard]] virtual std::vector<db_rule> get_deliberative_rules() noexcept;
    virtual void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content);

  protected:
    const json::json config;
  };
} // namespace coco
