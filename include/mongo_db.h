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

    virtual void init() override;

    std::string create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) override;
    void set_sensor_type_name(sensor_type &st, const std::string &name) override;
    void set_sensor_type_description(sensor_type &st, const std::string &description) override;
    void delete_sensor_type(sensor_type &st) override;

    std::string create_sensor(const std::string &name, const sensor_type &type, location_ptr l = nullptr) override;
    void set_sensor_name(sensor &s, const std::string &name) override;
    void set_sensor_location(sensor &s, location_ptr l) override;
    json::json get_sensor_values(sensor &s, const std::chrono::milliseconds::rep &start, const std::chrono::milliseconds::rep &end) override;
    void set_sensor_value(sensor &s, const std::chrono::milliseconds::rep &time, const json::json &val) override;
    void delete_sensor(sensor &s) override;

    void drop() override;

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
