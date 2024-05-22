#pragma once

#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include "coco_db.hpp"

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  class mongo_db : public coco_db
  {
  public:
    mongo_db(const json::json &config = {{"name", COCO_NAME}}, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    virtual ~mongo_db() = default;

    type &create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars) override;

    item &create_item(const type &tp, const std::string &name, const json::json &pars) override;

    void add_data(const item &it, const std::chrono::system_clock::time_point &timestamp, const json::json &data) override;

    rule &create_reactive_rule(const std::string &name, const std::string &content) override;

    rule &create_deliberative_rule(const std::string &name, const std::string &content) override;

  private:
    static bsoncxx::builder::basic::document to_bson(const parameter &p);
    static std::unique_ptr<parameter> from_bson(const bsoncxx::v_noabi::document::view &doc);

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
    mongocxx::collection items_collection;
    mongocxx::collection sensor_data_collection;
    mongocxx::collection reactive_rules_collection;
    mongocxx::collection deliberative_rules_collection;
  };
} // namespace coco