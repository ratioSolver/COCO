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
    mongo_db(const std::string &name = COCO_NAME, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));

    virtual void init() override;

    std::string create_instance(const std::string &name = COCO_NAME, const json::json &data = {}) override;

    std::string create_sensor_type(const std::string &name, const std::string &description, std::vector<parameter_ptr> &&parameters) override;
    void set_sensor_type_name(sensor_type &st, const std::string &name) override;
    void set_sensor_type_description(sensor_type &st, const std::string &description) override;
    void delete_sensor_type(sensor_type &st) override;

    std::string create_sensor(const std::string &name, sensor_type &type, location_ptr l = nullptr) override;
    void set_sensor_name(sensor &s, const std::string &name) override;
    void set_sensor_location(sensor &s, location_ptr l) override;
    json::json get_last_sensor_value(sensor &s) override;
    json::json get_sensor_data(sensor &s, const std::chrono::system_clock::time_point &start, const std::chrono::system_clock::time_point &end) override;
    void set_sensor_data(sensor &s, const std::chrono::system_clock::time_point &timestamp, const json::json &val) override;
    void delete_sensor(sensor &s) override;

    void drop() override;

    static parameter_ptr to_par(const bsoncxx::v_noabi::document::view &doc);
    static bsoncxx::v_noabi::builder::basic::document to_bson(const parameter &p);

  private:
    mongocxx::client conn;

  protected:
    mongocxx::v_noabi::database db;

  private:
    mongocxx::v_noabi::collection instances_collection;
    mongocxx::v_noabi::collection sensor_types_collection;
    mongocxx::v_noabi::collection sensors_collection;
    mongocxx::v_noabi::collection sensor_data_collection;
  };
} // namespace coco
