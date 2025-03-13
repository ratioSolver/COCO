#pragma once

#include "coco.hpp"
#include "server.hpp"
#include <unordered_set>

namespace coco
{
  class coco_server : public listener, public network::server
  {
  public:
    coco_server(coco &cc, std::string_view host = SERVER_HOST, unsigned short port = SERVER_PORT);

  protected:
    utils::u_ptr<network::response> index(const network::request &req);
    utils::u_ptr<network::response> assets(const network::request &req);

    utils::u_ptr<network::response> get_types(const network::request &req);
    utils::u_ptr<network::response> get_type(const network::request &req);
    utils::u_ptr<network::response> create_type(const network::request &req);
    utils::u_ptr<network::response> delete_type(const network::request &req);

  private:
    virtual void on_ws_open(network::ws_session &ws);
    virtual void on_ws_message(network::ws_session &ws, std::string_view msg);
    virtual void on_ws_close(network::ws_session &ws);
    virtual void on_ws_error(network::ws_session &ws, const std::error_code &);

    void new_type(const type &tp) override;
    void new_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

  protected:
    void broadcast(json::json &&msg);

  private:
    std::unordered_set<network::ws_session *> clients;
  };
} // namespace coco
