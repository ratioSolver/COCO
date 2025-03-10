#pragma once

#include "coco_db.hpp"
#include <mongocxx/client.hpp>

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  class mongo_db : public coco_db
  {
  public:
    mongo_db(json::json &&cnfg = {{"name", COCO_NAME}}, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) noexcept;

    void drop() noexcept override;

    std::vector<db_type> get_types() noexcept override;
    void create_type(std::string_view name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props) override;
    void set_parents(std::string_view name, const std::vector<std::string> &parents) override;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
  };
} // namespace coco
