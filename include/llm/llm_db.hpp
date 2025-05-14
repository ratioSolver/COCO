#pragma once

#include "mongo_db.hpp"

namespace coco
{
  struct db_intent
  {
    std::string name, description;
  };

  struct db_entity
  {
    std::string name, description;
    int type;
  };

  class llm_db : public mongo_module
  {
  public:
    llm_db(mongo_db &db) noexcept;

    [[nodiscard]] std::vector<db_intent> get_intents() noexcept;
    void create_intent(std::string_view name, std::string_view description);

    [[nodiscard]] std::vector<db_entity> get_entities() noexcept;
    void create_entity(std::string_view name, std::string_view description, int type);

  private:
    mongocxx::collection intents_collection, entities_collection;
  };
} // namespace coco
