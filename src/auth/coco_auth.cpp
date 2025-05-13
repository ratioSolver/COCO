#include "coco_auth.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "auth_db.hpp"
#include "logging.hpp"

namespace coco
{
    server_noauth::server_noauth(coco_server &srv) noexcept : server_module(srv), listener(get_coco())
    {
        srv.add_ws_route("/coco").on_open(std::bind(&server_noauth::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server_noauth::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server_noauth::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server_noauth::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

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

    void server_noauth::broadcast(json::json &&msg)
    {
        auto msg_str = msg.dump();
        for (auto client : clients)
            client->send(msg_str);
    }

    coco_auth::coco_auth(coco &cc) noexcept : coco_module(cc)
    {
        try
        {
            [[maybe_unused]] auto &tp = cc.get_type(user_kw);
        }
        catch (const std::invalid_argument &)
        { // Type does not exist, create it
            [[maybe_unused]] auto &tp = cc.create_type(user_kw, {}, {}, {}, {});
        }
        cc.get_db().add_module<auth_db>(static_cast<mongo_db &>(cc.get_db()));
    }

    std::string coco_auth::get_token(std::string_view username, std::string_view password)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto user = cc.get_db().get_module<auth_db>().get_user(username, password);
        return user.id;
    }

    item &coco_auth::create_user(std::string_view username, std::string_view password, json::json &&personal_data)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto &tp = cc.get_type(user_kw);
        auto &itm = cc.create_item(tp);
        cc.get_db().get_module<auth_db>().create_user(itm.get_id(), username, password, std::move(personal_data));
        return itm;
    }

    server_auth::server_auth(coco_server &srv) noexcept : server_module(srv), listener(get_coco())
    {
        srv.add_route(network::verb::Post, "^/login$", std::bind(&server_auth::login, this, std::placeholders::_1));

        srv.add_ws_route("/coco").on_open(std::bind(&server_auth::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server_auth::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server_auth::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server_auth::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    utils::u_ptr<network::response> server_auth::login(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("username") || body["username"].get_type() != json::json_type::string || !body.contains("password") || body["password"].get_type() != json::json_type::string)
            return utils::make_u_ptr<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string username = body["username"];
        std::string password = body["password"];

        try
        {
            auto token = get_coco().get_module<coco_auth>().get_token(username, password);
            if (token.empty())
                return utils::make_u_ptr<network::json_response>(json::json({{"message", "Unauthorized"}}), network::status_code::unauthorized);
            return utils::make_u_ptr<network::json_response>(json::json({{"token", token.c_str()}}), network::status_code::ok);
        }
        catch (const std::invalid_argument &)
        {
            return utils::make_u_ptr<network::json_response>(json::json({{"message", "Invalid credentials"}}), network::status_code::unauthorized);
        }
    }
} // namespace coco
