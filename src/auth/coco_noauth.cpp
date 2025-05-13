#include "coco_noauth.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"

namespace coco
{
    server_noauth::server_noauth(coco_server &srv) noexcept : server_module(srv), listener(get_coco())
    {
        srv.add_ws_route("/coco").on_open(std::bind(&server_noauth::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server_noauth::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server_noauth::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server_noauth::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    void server_noauth::new_type(const type &tp)
    {
        auto j_tp = tp.to_json();
        j_tp["msg_type"] = "new_type";
        j_tp["name"] = tp.get_name();
        broadcast(std::move(j_tp));
    }
    void server_noauth::new_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "new_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void server_noauth::updated_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "updated_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void server_noauth::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) { broadcast({{"msg_type", "new_data"}, {"id", itm.get_id().c_str()}, {"value", {{"data", data}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()}}}}); }

    void server_noauth::on_ws_open(network::ws_session &ws)
    {
        LOG_TRACE("New connection from " << ws.remote_endpoint());

        std::lock_guard<std::mutex> _(mtx);
        clients.insert(&ws);

        auto jc = cc.to_json();
        jc["msg_type"] = "coco";
        ws.send(jc.dump());
    }
    void server_noauth::on_ws_message(network::ws_session &ws, std::string_view msg)
    {
        auto x = json::load(msg);
        if (x.get_type() != json::json_type::object || !x.contains("type") || x["type"].get_type() != json::json_type::string)
        {
            ws.close();
            return;
        }
    }
    void server_noauth::on_ws_close(network::ws_session &ws)
    {
        LOG_TRACE("Connection closed with " << ws.remote_endpoint());
        std::lock_guard<std::mutex> _(mtx);
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server_noauth::on_ws_error(network::ws_session &ws, [[maybe_unused]] const std::error_code &ec)
    {
        LOG_TRACE("Connection error with " << ws.remote_endpoint() << ": " << ec.message());
        on_ws_close(ws);
    }

    void server_noauth::broadcast(json::json &&msg)
    {
        auto msg_str = msg.dump();
        for (auto client : clients)
            client->send(msg_str);
    }
} // namespace coco
