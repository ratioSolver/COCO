#pragma once

#include "mongo_db.hpp"

namespace coco
{
  class fcm_db : public mongo_module
  {
  public:
    fcm_db(mongo_db &db) noexcept;

    void add_token(std::string_view id, std::string_view token);
    void remove_token(std::string_view id, std::string_view token);
    [[nodiscard]] std::vector<std::string> get_tokens(std::string_view id) noexcept;

    static constexpr const char *fcm_collection_name = "fcm_tokens";
  };
} // namespace coco
