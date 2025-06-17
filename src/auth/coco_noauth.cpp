#include "coco_noauth.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"

namespace coco
{
    server_noauth::server_noauth(coco_server &srv) noexcept : server_module(srv)
    {
        srv.add_ws_route("/coco").on_open(std::bind(&server_noauth::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server_noauth::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server_noauth::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server_noauth::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    void server_noauth::on_ws_open(network::ws_server_session_base &ws)
    {
        std::lock_guard<std::mutex> _(mtx);
        clients.insert(&ws);

        auto jc = get_coco().to_json();
        jc["msg_type"] = "coco";
        ws.send(jc.dump());
    }
    void server_noauth::on_ws_message(network::ws_server_session_base &ws, const network::message &msg)
    {
        auto x = json::load(msg.get_payload());
        if (x.get_type() != json::json_type::object || !x.contains("msg_type") || x["msg_type"].get_type() != json::json_type::string)
        {
            ws.close();
            return;
        }
    }
    void server_noauth::on_ws_close(network::ws_server_session_base &ws)
    {
        std::lock_guard<std::mutex> _(mtx);
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server_noauth::on_ws_error(network::ws_server_session_base &ws, [[maybe_unused]] const std::error_code &ec)
    {
        LOG_TRACE("Connection error: " << ec.message());
        on_ws_close(ws);
    }

    void server_noauth::broadcast(json::json &msg)
    {
        auto msg_str = msg.dump();
        for (auto client : clients)
            client->send(msg_str);
    }
} // namespace coco
