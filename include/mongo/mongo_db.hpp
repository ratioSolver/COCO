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
    mongo_db(const json::json &config, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    virtual ~mongo_db() = default;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::v_noabi::database db;

  private:
    mongocxx::v_noabi::collection sensor_types_collection;
    mongocxx::v_noabi::collection sensors_collection;
    mongocxx::v_noabi::collection sensor_data_collection;
  };
} // namespace coco