#pragma once

#include "coco_server.hpp"
#include "coco.hpp"
#include "coco_module.hpp"
#include "coco_item.hpp"

namespace coco
{
  constexpr const char *user_kw = "User";

  class server_noauth : public server_module, public listener
  {
  public:
    server_noauth(coco_server &srv) noexcept;

  private:
    void new_type(const type &tp) override;
    void new_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

    void on_ws_open(network::ws_session &ws);
    void on_ws_message(network::ws_session &ws, std::string_view msg);
    void on_ws_close(network::ws_session &ws);
    void on_ws_error(network::ws_session &ws, const std::error_code &);

    void broadcast(json::json &&msg);

  private:
    std::mutex mtx;
    std::unordered_set<network::ws_session *> clients;
  };

  class coco_auth : public coco_module
  {
  public:
    coco_auth(coco &cc) noexcept;

    [[nodiscard]] std::string get_token(std::string_view username, std::string_view password);

    [[nodiscard]] item &create_user(std::string_view username, std::string_view password, json::json &&personal_data = {});
  };

  class server_auth : public server_module, public listener
  {
  public:
    server_auth(coco_server &srv) noexcept;

  private:
    utils::u_ptr<network::response> login(const network::request &req);

  private:
    void new_type(const type &tp) override;
    void new_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

    void on_ws_open(network::ws_session &ws);
    void on_ws_message(network::ws_session &ws, std::string_view msg);
    void on_ws_close(network::ws_session &ws);
    void on_ws_error(network::ws_session &ws, const std::error_code &);

    void broadcast(json::json &&msg);

  private:
    std::mutex mtx;
    std::unordered_set<network::ws_session *> clients;
  };
} // namespace coco
