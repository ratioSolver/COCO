#pragma once

#include "coco_db.h"
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>

#define MONGODB_URI(host, port) "mongodb://" host ":" port

namespace coco
{
  class mongo_db : public coco_db
  {
  public:
    mongo_db(const std::string &root = COCO_ROOT, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));

    void init() override;

    std::string create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) override;
    void set_sensor_type_name(const std::string &id, const std::string &name) override;
    void set_sensor_type_description(const std::string &id, const std::string &description) override;
    void delete_sensor_type(const std::string &id) override;

    std::string create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l = nullptr) override;
    void set_sensor_name(const std::string &id, const std::string &name) override;
    void set_sensor_location(const std::string &id, std::unique_ptr<location> l) override;
    void set_sensor_value(const std::string &id, const std::chrono::milliseconds::rep &time, const json::json &val) override;
    void delete_sensor(const std::string &id) override;

    void drop() override;

  private:
    mongocxx::client conn;
    mongocxx::v_noabi::database db;
    mongocxx::v_noabi::collection sensor_types_collection;
    mongocxx::v_noabi::collection sensors_collection;
    mongocxx::v_noabi::collection sensor_data_collection;
  };
} // namespace coco
