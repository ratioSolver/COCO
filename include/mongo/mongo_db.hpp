#pragma once

#include <mongocxx/instance.hpp>
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
    mongo_db(const json::json &config = {{"name", COCO_NAME}}, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    virtual ~mongo_db() = default;

    [[nodiscard]] type &create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties) override;

    void set_type_name(type &tp, const std::string &name) override;
    void set_type_description(type &tp, const std::string &description) override;
    void add_static_property(type &tp, std::unique_ptr<property> &&prop) override;
    void remove_static_property(type &tp, const property &prop) override;
    void add_dynamic_property(type &tp, std::unique_ptr<property> &&prop) override;
    void remove_dynamic_property(type &tp, const property &prop) override;
    void delete_type(const type &tp) override;

    [[nodiscard]] item &create_item(const type &tp, const std::string &name, const json::json &props) override;

    void set_item_name(item &itm, const std::string &name) override;
    void set_item_properties(item &itm, const json::json &props) override;
    void delete_item(const item &itm) override;

    void add_data(const item &itm, const std::chrono::system_clock::time_point &timestamp, const json::json &data) override;

    [[nodiscard]] rule &create_reactive_rule(const std::string &name, const std::string &content) override;

    [[nodiscard]] rule &create_deliberative_rule(const std::string &name, const std::string &content) override;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
    mongocxx::collection propertys_collection;
    mongocxx::collection items_collection;
    mongocxx::collection item_data_collection;
    mongocxx::collection reactive_rules_collection;
    mongocxx::collection deliberative_rules_collection;
  };
} // namespace coco