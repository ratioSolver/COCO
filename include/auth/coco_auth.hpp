#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"

namespace coco
{
  constexpr const char *user_kw = "User";

  class authentication : public coco_module
  {
  public:
    authentication(coco &cc) noexcept;

    [[nodiscard]] std::string get_token(std::string_view username, std::string_view password);

    [[nodiscard]] item &create_user(std::string_view username, std::string_view password, json::json &&personal_data = {});
  };
} // namespace coco
