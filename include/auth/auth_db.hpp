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

    [[nodiscard]] db_user get_user(std::string_view username, std::string_view password);

    void create_user(std::string_view itm_id, std::string_view username, std::string_view password, json::json &&personal_data = {});

  private:
    mongocxx::client users_conn;
    mongocxx::database users_db;
    mongocxx::collection users_collection;
  };
} // namespace coco
