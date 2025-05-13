#pragma once

#include "coco_db.hpp"
#include <mongocxx/client.hpp>

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  class mongo_db;

  class mongo_module : public db_module
  {
  public:
    mongo_module(mongo_db &db) noexcept;

  protected:
    [[nodiscard]] mongocxx::database &get_db() const noexcept;
  };

  class mongo_db : public coco_db
  {
    friend class mongo_module;

  public:
    mongo_db(json::json &&cnfg = {{"name", COCO_NAME}}, std::string_view mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) noexcept;

    void drop() noexcept override;

    [[nodiscard]] std::vector<db_type> get_types() noexcept override;
    void create_type(std::string_view tp_name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props) override;
    void set_parents(std::string_view tp_name, const std::vector<std::string> &parents) override;
    void delete_type(std::string_view tp_name) override;

    [[nodiscard]] std::vector<db_item> get_items() noexcept override;
    std::string create_item(std::string_view type, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val = std::nullopt) override;
    [[nodiscard]] json::json get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to = std::chrono::system_clock::now()) override;
    void set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) override;
    void delete_item(std::string_view itm_id) override;

    [[nodiscard]] std::vector<db_rule> get_reactive_rules() noexcept override;
    void create_reactive_rule(std::string_view rule_name, std::string_view rule_content) override;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection, items_collection, item_data_collection, reactive_rules_collection;
  };
} // namespace coco
