#pragma once

#include "coco_server.hpp"
#include "coco_module.hpp"
#include "coco_item.hpp"

namespace coco
{
  constexpr const char *user_kw = "User";

  class coco_auth : public coco_module
  {
  public:
    coco_auth(coco &cc) noexcept;

    [[nodiscard]] std::string get_token(std::string_view username, std::string_view password);

    [[nodiscard]] item &create_user(std::string_view username, std::string_view password, json::json &&personal_data = {});
  };

  class server_auth : public server_module
  {
  public:
    server_auth(coco_server &srv) noexcept;

  private:
    utils::u_ptr<network::response> login(const network::request &req);

  private:
    void on_ws_open(network::ws_session &ws) override;
    void on_ws_message(network::ws_session &ws, std::string_view msg) override;
    void on_ws_close(network::ws_session &ws) override;
    void on_ws_error(network::ws_session &ws, const std::error_code &) override;

    void broadcast(json::json &msg) override;

  private:
    std::mutex mtx;
    std::unordered_set<network::ws_session *> clients;
  };
} // namespace coco
