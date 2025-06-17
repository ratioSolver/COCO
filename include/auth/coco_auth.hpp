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

    [[nodiscard]] bool is_valid_token(std::string_view token) const noexcept;
    [[nodiscard]] std::string get_token(std::string_view username, std::string_view password);

    [[nodiscard]] std::vector<std::reference_wrapper<item>> get_users() noexcept;
    [[nodiscard]] item &create_user(std::string_view username, std::string_view password, json::json &&personal_data = {});
  };

  class server_auth : public server_module
  {
  public:
    server_auth(coco_server &srv) noexcept;

  private:
    std::unique_ptr<network::response> login(const network::request &req);
    std::unique_ptr<network::response> get_users(const network::request &req);
    std::unique_ptr<network::response> create_user(const network::request &req);

  private:
    void on_ws_open(network::ws_server_session_base &ws) override;
    void on_ws_message(network::ws_server_session_base &ws, const network::message &msg) override;
    void on_ws_close(network::ws_server_session_base &ws) override;
    void on_ws_error(network::ws_server_session_base &ws, const std::error_code &) override;

    void broadcast(json::json &msg) override;

  private:
    std::mutex mtx;
    std::unordered_map<network::ws_server_session_base *, std::string> clients;
  };
} // namespace coco
