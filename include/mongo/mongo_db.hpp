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

    type &create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars) override;
    item &create_item(const type &tp, const std::string &name, json::json &&data = {}) override;

  private:
    static bsoncxx::builder::basic::document to_bson(const parameter &p);

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
    mongocxx::collection items_collection;
    mongocxx::collection sensor_data_collection;
  };
} // namespace coco