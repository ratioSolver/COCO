#pragma once

#include "coco_db.hpp"
#include <mongocxx/client.hpp>

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  class mongo_db : public coco_db
  {
  public:
#ifdef BUILD_AUTH
    mongo_db(json::json &&cnfg = {{ "name",
                                    COCO_NAME }},
             std::string_view mongodb_users_uri = MONGODB_URI(MONGODB_USERS_HOST, MONGODB_USERS_PORT), std::string_view mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) noexcept;
#else
    mongo_db(json::json &&cnfg = {{"name", COCO_NAME}}, std::string_view mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) noexcept;
#endif

    void drop() noexcept override;

#ifdef BUILD_AUTH
    [[nodiscard]] db_user get_user(std::string_view username, std::string_view password) override;

    void create_user(std::string_view itm_id, std::string_view username, std::string_view password, json::json &&personal_data = {}) override;
#endif

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
    [[nodiscard]] std::vector<db_rule> get_deliberative_rules() noexcept override;
    void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content) override;

  private:
    mongocxx::client conn;
#ifdef BUILD_AUTH
    mongocxx::client users_conn;
#endif

  protected:
    mongocxx::database db;
#ifdef BUILD_AUTH
    mongocxx::database users_db;
#endif

  private:
    mongocxx::collection types_collection, items_collection, item_data_collection, reactive_rules_collection, deliberative_rules_collection;
#ifdef BUILD_AUTH
    mongocxx::collection users_collection;
#endif
  };
} // namespace coco
