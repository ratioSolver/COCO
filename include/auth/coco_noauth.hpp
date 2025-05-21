#pragma once

#include "coco_server.hpp"
#include "coco_module.hpp"
#include "coco_item.hpp"

namespace coco
{
  class server_noauth : public server_module
  {
  public:
    server_noauth(coco_server &srv) noexcept;

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
