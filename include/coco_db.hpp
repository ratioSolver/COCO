#pragma once

#include "json.hpp"
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <optional>
#include <chrono>

namespace coco
{
  class coco_db;

  struct db_type
  {
    std::string name;
    std::optional<json::json> static_props, dynamic_props, data;
  };

  struct db_item
  {
    std::string id;
    std::vector<std::string> types;
    std::optional<json::json> props;
    std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value;
  };

  struct db_rule
  {
    std::string name, content;
  };

  class db_module
  {
    friend class coco_db;

  public:
    db_module(coco_db &db) noexcept;
    virtual ~db_module() = default;

  private:
    virtual void drop() noexcept;

  protected:
    coco_db &db;
  };

  class coco_db
  {
  public:
    coco_db(json::json &&cnfg = {}) noexcept;

    [[nodiscard]] const json::json &get_config() const noexcept { return config; }

    template <typename Tp, typename... Args>
    Tp &add_module(Args &&...args)
    {
      static_assert(std::is_base_of<db_module, Tp>::value, "Module must be derived from db_module");
      if (auto it = modules.find(typeid(Tp)); it == modules.end())
      {
        auto mod = std::make_unique<Tp>(std::forward<Args>(args)...);
        auto &ref = *mod;
        modules.emplace(typeid(Tp), std::move(mod));
        return ref;
      }
      else
        throw std::runtime_error("Module already exists");
    }

    template <typename Tp>
    [[nodiscard]] Tp &get_module() const
    {
      static_assert(std::is_base_of<db_module, Tp>::value, "Module must be derived from db_module");
      if (auto it = modules.find(typeid(Tp)); it != modules.end())
        return *static_cast<Tp *>(it->second.get());
      throw std::runtime_error("Module not found");
    }

    virtual void drop() noexcept;

    [[nodiscard]] virtual std::vector<db_type> get_types() noexcept;
    virtual void create_type(std::string_view tp_name, const json::json &static_props, const json::json &dynamic_props, const json::json &data);
    virtual void set_properties(std::string_view tp_name, const json::json &static_props, const json::json &dynamic_props);
    virtual void delete_type(std::string_view tp_name);

    [[nodiscard]] virtual std::vector<db_item> get_items() noexcept;
    virtual std::string create_item(const std::vector<std::string> &types, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val = std::nullopt);
    virtual void set_properties(std::string_view itm_id, const json::json &props);
    [[nodiscard]] virtual json::json get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to = std::chrono::system_clock::now());
    virtual void set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now());
    virtual void delete_item(std::string_view itm_id);

    [[nodiscard]] virtual std::vector<db_rule> get_rules() noexcept;
    virtual void create_rule(std::string_view rule_name, std::string_view rule_content);

  protected:
    const json::json config;

  private:
    std::unordered_map<std::type_index, std::unique_ptr<db_module>> modules; // The modules..
  };
} // namespace coco
