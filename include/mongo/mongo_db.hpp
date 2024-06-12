#pragma once

#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>
#include "coco_db.hpp"
#include "bson_parameter_converter.hpp"

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

    [[nodiscard]] type &create_type(const std::string &name, const std::string &description, std::unordered_map<std::string, std::unique_ptr<coco_parameter>> &&static_pars, std::unordered_map<std::string, std::unique_ptr<coco_parameter>> &&dynamic_pars) override;

    void set_type_name(type &type, const std::string &name) override;
    void set_type_description(type &type, const std::string &description) override;
    void add_static_parameter(type &type, std::unique_ptr<coco_parameter> &&par) override;
    void remove_static_parameter(type &type, const std::string &name) override;
    void add_dynamic_parameter(type &type, std::unique_ptr<coco_parameter> &&par) override;
    void remove_dynamic_parameter(type &type, const std::string &name) override;
    void delete_type(const type &type) override;

    [[nodiscard]] item &create_item(const type &tp, const std::string &name, const json::json &pars) override;

    void set_item_name(item &it, const std::string &name) override;
    void set_item_parameters(item &it, const json::json &pars) override;
    void delete_item(const item &it) override;

    void add_data(const item &it, const std::chrono::system_clock::time_point &timestamp, const json::json &data) override;

    [[nodiscard]] rule &create_reactive_rule(const std::string &name, const std::string &content) override;

    [[nodiscard]] rule &create_deliberative_rule(const std::string &name, const std::string &content) override;

  protected:
    template <typename Tp, typename... Args>
    void new_parameter_converter(Args &&...args) noexcept
    {
      static_assert(std::is_base_of<bson_parameter_converter, Tp>::value, "Tp must be a subclass of bson_parameter_converter");
      auto tp = std::make_unique<Tp>(std::forward<Args>(args)...);
      converters[tp->get_type()] = std::move(tp);
    }

  private:
    [[nodiscard]] bsoncxx::builder::basic::document to_bson(const coco_parameter &p) const { return converters.at(p.get_type())->to_bson(p); }
    [[nodiscard]] std::unique_ptr<coco_parameter> from_bson(const bsoncxx::v_noabi::document::view &doc) const { return converters.at(doc["type"].get_string().value.to_string())->from_bson(doc); }

  private:
    mongocxx::client conn;

  protected:
    mongocxx::database db;

  private:
    mongocxx::collection types_collection;
    mongocxx::collection items_collection;
    mongocxx::collection item_data_collection;
    mongocxx::collection reactive_rules_collection;
    mongocxx::collection deliberative_rules_collection;
    std::unordered_map<std::string, std::unique_ptr<bson_parameter_converter>> converters;
  };
} // namespace coco