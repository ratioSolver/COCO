#pragma once

#include "mongo_db.hpp"

namespace coco
{
  struct db_user
  {
    std::string id, username;
    json::json personal_data;
  };

  class auth_db : public mongo_module
  {
  public:
    auth_db(mongo_db &db, std::string_view mongodb_users_uri = MONGODB_URI(MONGODB_USERS_HOST, MONGODB_USERS_PORT)) noexcept;

    [[nodiscard]] db_user get_user(std::string_view id);

    [[nodiscard]] db_user get_user(std::string_view username, std::string_view password);

    [[nodiscard]] std::vector<db_user> get_users() noexcept;

    void create_user(std::string_view id, std::string_view username, std::string_view password, json::json &&personal_data = {});

  private:
    void drop() noexcept override;

  private:
    mongocxx::client users_conn;
    mongocxx::database users_db;
    mongocxx::collection users_collection;
  };
} // namespace coco
