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
    int type;
    std::string name;
    std::string description;
  };

  struct db_slot
  {
    int type;
    std::string name;
    std::string description;
    bool influence_context;
  };

  class llm_db : public mongo_module
  {
  public:
    llm_db(mongo_db &db) noexcept;

    [[nodiscard]] std::vector<db_intent> get_intents() noexcept;
    void create_intent(std::string_view name, std::string_view description);

    [[nodiscard]] std::vector<db_entity> get_entities() noexcept;
    void create_entity(int type, std::string_view name, std::string_view description);

    [[nodiscard]] std::vector<db_slot> get_slots() noexcept;
    void create_slot(int type, std::string_view name, std::string_view description, bool influence_context = true);

  private:
    mongocxx::collection intents_collection, entities_collection, slots_collection;
  };
} // namespace coco
