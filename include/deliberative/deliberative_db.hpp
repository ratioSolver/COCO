#pragma once

#include "mongo_db.hpp"

namespace coco
{
  class deliberative_db : public mongo_module
  {
  public:
    deliberative_db(mongo_db &db) noexcept;

    [[nodiscard]] std::vector<db_rule> get_deliberative_rules() noexcept;
    void create_deliberative_rule(std::string_view rule_name, std::string_view rule_content);

  private:
    mongocxx::collection deliberative_rules_collection;
  };
} // namespace coco
