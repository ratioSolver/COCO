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
    friend class bson_parameter_converter;

  public:
    mongo_db(const json::json &config = {{"name", COCO_NAME}}, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT));
    virtual ~mongo_db() = default;

    [[nodiscard]] parameter &create_parameter(const std::string &name, const std::string &description, json::json &&schema) override;

    void set_parameter_name(parameter &par, const std::string &name) override;
    void set_parameter_description(parameter &par, const std::string &description) override;
    void add_super_parameter(parameter &par, const parameter &super_par) override;
    void remove_super_parameter(parameter &par, const parameter &super_par) override;
    void add_disjoint_parameter(parameter &par, const parameter &disjoint_par) override;
    void remove_disjoint_parameter(parameter &par, const parameter &disjoint_par) override;
    void set_parameter_schema(parameter &par, json::json &&schema) override;
    void delete_parameter(const parameter &par) override;

    [[nodiscard]] type &create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const parameter>> &&static_pars, std::vector<std::reference_wrapper<const parameter>> &&dynamic_pars) override;

    void set_type_name(type &tp, const std::string &name) override;
    void set_type_description(type &tp, const std::string &description) override;
    void add_super_type(type &tp, const type &super_type) override;
    void remove_super_type(type &tp, const type &super_type) override;
    void add_disjoint_type(type &tp, const type &disjoint_type) override;
    void remove_disjoint_type(type &tp, const type &disjoint_type) override;
    void add_static_parameter(type &tp, const parameter &par) override;
    void remove_static_parameter(type &tp, const parameter &par) override;
    void add_dynamic_parameter(type &tp, const parameter &par) override;
    void remove_dynamic_parameter(type &tp, const parameter &par) override;
    void delete_type(const type &tp) override;

    [[nodiscard]] item &create_item(const type &tp, const std::string &name, const json::json &pars) override;

    void set_item_name(item &it, const std::string &name) override;
    void set_item_parameters(item &it, const json::json &pars) override;
    void delete_item(const item &it) override;

    void add_data(const item &it, const std::chrono::system_clock::time_point &timestamp, const json::json &data) override;

    [[nodiscard]] rule &create_reactive_rule(const std::string &name, const std::string &content) override;

    [[nodiscard]] rule &create_deliberative_rule(const std::string &name, const std::string &content) override;

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
    mongocxx::collection parameters_collection;
    mongocxx::collection items_collection;
    mongocxx::collection item_data_collection;
    mongocxx::collection reactive_rules_collection;
    mongocxx::collection deliberative_rules_collection;
  };
} // namespace coco