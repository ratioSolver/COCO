#pragma once

#include "coco_db.hpp"
#include <mongocxx/client.hpp>

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

  [[nodiscard]] inline std::string default_mongodb_uri() noexcept
  {
    std::string uri = "mongodb://";
#ifdef MONGODB_AUTH
    const char *user = std::getenv("MONGODB_USER");
    if (user)
      uri += user;
    uri += ":";
    const char *pass = std::getenv("MONGODB_PASSWORD");
    if (pass)
      uri += pass;
    uri += "@";
#endif
    const char *host = std::getenv("MONGODB_HOST");
    if (host)
      uri += host;
    else
      uri += "localhost";
    uri += ":";
    const char *port = std::getenv("MONGODB_PORT");
    if (port)
      uri += port;
    else
      uri += "27017";
    return uri;
  }

  class mongo_db : public coco_db
  {
    friend class mongo_module;

  public:
    mongo_db(json::json &&cnfg = {{"name", COCO_NAME}}, std::string_view mongodb_uri = default_mongodb_uri()) noexcept;

    void drop() noexcept override;

    [[nodiscard]] std::vector<db_type> get_types() noexcept override;
    void create_type(std::string_view tp_name, const json::json &static_props, const json::json &dynamic_props, const json::json &data) override;
    void delete_type(std::string_view tp_name) override;

    [[nodiscard]] std::vector<db_item> get_items() noexcept override;
    std::string create_item(const std::vector<std::string> &types, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val = std::nullopt) override;
    void set_properties(std::string_view itm_id, const json::json &props) override;
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

  [[nodiscard]] bsoncxx::array::value to_bson_array(const json::json &j);
} // namespace coco
