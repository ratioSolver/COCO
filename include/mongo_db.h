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
    mongo_db(const std::string &instance = COCO_INSTANCE, const std::string &mongodb_instance_uri = MONGODB_URI(MONGODB_INSTANCE_HOST, MONGODB_INSTANCE_PORT), const std::string &app = COCO_APP, const std::string &mongodb_app_uri = MONGODB_URI(MONGODB_APP_HOST, MONGODB_APP_PORT));

    virtual void init() override;

    std::string create_user(bool admin, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const json::json &data = {}) override;
    user_ptr get_user(const std::string &email, const std::string &password) override;
    std::vector<user_ptr> get_all_users() override;
    void set_user_first_name(user &u, const std::string &first_name) override;
    void set_user_first_name(const std::string &id, const std::string &first_name) override;
    void set_user_last_name(user &u, const std::string &last_name) override;
    void set_user_last_name(const std::string &id, const std::string &last_name) override;
    void set_user_email(user &u, const std::string &email) override;
    void set_user_email(const std::string &id, const std::string &email) override;
    void set_user_password(user &u, const std::string &password) override;
    void set_user_password(const std::string &id, const std::string &password) override;
    void set_user_data(user &u, const json::json &data) override;
    void set_user_data(const std::string &id, const json::json &data) override;
    void delete_user(user &u) override;
    void delete_user(const std::string &id) override;

    std::string create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) override;
    void set_sensor_type_name(sensor_type &st, const std::string &name) override;
    void set_sensor_type_description(sensor_type &st, const std::string &description) override;
    void delete_sensor_type(sensor_type &st) override;

    std::string create_sensor(const std::string &name, sensor_type &type, location_ptr l = nullptr) override;
    void set_sensor_name(sensor &s, const std::string &name) override;
    void set_sensor_location(sensor &s, location_ptr l) override;
    json::json get_last_sensor_value(sensor &s) override;
    json::json get_sensor_values(sensor &s, const std::chrono::system_clock::time_point &start, const std::chrono::system_clock::time_point &end) override;
    void set_sensor_value(sensor &s, const std::chrono::system_clock::time_point &time, const json::json &val) override;
    void delete_sensor(sensor &s) override;

    void drop() override;

  private:
    mongocxx::client app_conn;
    mongocxx::client instance_conn;

  protected:
    mongocxx::v_noabi::database app_db;
    mongocxx::v_noabi::database instance_db;

  private:
    mongocxx::v_noabi::collection users_collection;
    mongocxx::v_noabi::collection sensor_types_collection;
    mongocxx::v_noabi::collection sensors_collection;
    mongocxx::v_noabi::collection sensor_data_collection;
  };
} // namespace coco
