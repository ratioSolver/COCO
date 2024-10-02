#pragma once

#include <mongocxx/client.hpp>
#include "coco_db.hpp"

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  /**
   * @brief Represents a MongoDB database.
   *
   * This class provides methods to interact with a MongoDB database.
   */
  class mongo_db : public coco_db
  {
    friend class bson_property_converter;

  public:
#ifdef ENABLE_AUTH
    mongo_db(const json::json &config = {{ "name",
                                           COCO_NAME }},
             const std::string &mongodb_users_uri = MONGODB_URI(MONGODB_USERS_HOST, MONGODB_USERS_PORT), const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
#else
    mongo_db(const json::json &config = {{"name", COCO_NAME}}, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
#endif

    void init(coco_core &cc) override;

#ifdef ENABLE_AUTH
    [[nodiscard]] item &create_user(coco_core &cc, const std::string &username, const std::string &password, json::json &&personal_data = {}, json::json &&data = {}) override;
    [[nodiscard]] std::optional<std::reference_wrapper<item>> get_user(const std::string &username, const std::string &password) override;
    void set_user_username(item &usr, const std::string &username) override;
    void set_user_password(item &usr, const std::string &password) override;
    void set_user_personal_data(item &usr, json::json &&personal_data) override;
    void delete_user(const item &usr) override;
#endif

    [[nodiscard]] type &create_type(coco_core &cc, const std::string &name, const std::string &description, json::json &&props) override;

    void set_type_name(type &tp, const std::string &name) override;
    void set_type_description(type &tp, const std::string &description) override;
    void set_type_properties(type &tp, json::json &&props) override;
    void set_type_parents(type &tp, std::vector<std::reference_wrapper<const type>> &&parents) override;
    void set_type_static_properties(type &tp, std::vector<std::unique_ptr<property>> &&props) override;
    void set_type_dynamic_properties(type &tp, std::vector<std::unique_ptr<property>> &&props) override;
    void delete_type(const type &tp) override;

    [[nodiscard]] item &create_item(coco_core &cc, const type &tp, json::json &&props, const json::json &val = json::json(), const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) override;

    void set_item_properties(item &itm, json::json &&props) override;
    void set_item_value(item &itm, const json::json &value, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) override;
    void delete_item(const item &itm) override;

    [[nodiscard]] json::json get_data(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to) override;
    void add_data(item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp = std::chrono::system_clock::now()) override;

    [[nodiscard]] rule &create_reactive_rule(coco_core &cc, const std::string &name, const std::string &content) override;
    void set_reactive_rule_name(rule &rl, const std::string &name) override;
    void set_reactive_rule_content(rule &rl, const std::string &content) override;
    void delete_reactive_rule(const rule &rl) override;

    [[nodiscard]] rule &create_deliberative_rule(coco_core &cc, const std::string &name, const std::string &content) override;
    void set_deliberative_rule_name(rule &rl, const std::string &name) override;
    void set_deliberative_rule_content(rule &rl, const std::string &content) override;
    void delete_deliberative_rule(const rule &rl) override;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
#ifdef ENABLE_AUTH
    mongocxx::client users_conn;
    mongocxx::database users_db;
    mongocxx::collection users_collection;
#endif
    mongocxx::collection types_collection;
    mongocxx::collection propertys_collection;
    mongocxx::collection items_collection;
    mongocxx::collection item_data_collection;
    mongocxx::collection reactive_rules_collection;
    mongocxx::collection deliberative_rules_collection;
  };
} // namespace coco