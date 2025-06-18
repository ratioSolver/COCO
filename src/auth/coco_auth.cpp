#include "coco_auth.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "auth_db.hpp"
#include "logging.hpp"

namespace coco
{
    coco_auth::coco_auth(coco &cc) noexcept : coco_module(cc)
    {
        try
        {
            [[maybe_unused]] auto &tp = get_coco().get_type(user_kw);
        }
        catch (const std::invalid_argument &)
        { // Type does not exist, create it
            [[maybe_unused]] auto &tp = get_coco().create_type(user_kw, {}, {}, {}, {});
        }
        get_coco().get_db().add_module<auth_db>(static_cast<mongo_db &>(get_coco().get_db()));
    }

    bool coco_auth::is_valid_token(std::string_view token) const noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        try
        {
            [[maybe_unused]] auto user = get_coco().get_db().get_module<auth_db>().get_user(token);
            return true;
        }
        catch (const std::invalid_argument &)
        {
            return false;
        }
    }

    std::string coco_auth::get_token(std::string_view username, std::string_view password)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto user = get_coco().get_db().get_module<auth_db>().get_user(username, password);
        return user.id;
    }

    std::vector<std::reference_wrapper<item>> coco_auth::get_users() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto &tp = get_coco().get_type(user_kw);
        std::vector<std::reference_wrapper<item>> users;
        for (auto &itm : get_coco().get_items(tp))
            users.push_back(itm);
        return users;
    }

    item &coco_auth::create_user(std::string_view username, std::string_view password, json::json &&personal_data)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto &tp = get_coco().get_type(user_kw);
        auto &itm = get_coco().create_item(tp);
        get_coco().get_db().get_module<auth_db>().create_user(itm.get_id(), username, password, std::move(personal_data));
        return itm;
    }

    server_auth::server_auth(coco_server &srv) noexcept : server_module(srv)
    {
        srv.add_route(network::verb::Post, "^/login$", std::bind(&server_auth::login, this, std::placeholders::_1));
        srv.add_route(network::verb::Get, "^/users$", std::bind(&server_auth::get_users, this, std::placeholders::_1));
        srv.add_route(network::verb::Post, "^/users$", std::bind(&server_auth::create_user, this, std::placeholders::_1));

        srv.add_ws_route("/coco").on_open(std::bind(&server_auth::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&server_auth::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&server_auth::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&server_auth::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> server_auth::login(const network::request &req)
    {
        LOG_TRACE("Login request received");
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("username") || body["username"].get_type() != json::json_type::string || !body.contains("password") || body["password"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string username = body["username"];
        std::string password = body["password"];

        try
        {
            auto token = get_coco().get_module<coco_auth>().get_token(username, password);
            if (token.empty())
                return std::make_unique<network::json_response>(json::json({{"message", "Unauthorized"}}), network::status_code::unauthorized);
            return std::make_unique<network::json_response>(json::json({{"token", token.c_str()}}), network::status_code::ok);
        }
        catch (const std::invalid_argument &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid credentials"}}), network::status_code::unauthorized);
        }
    }

    std::unique_ptr<network::response> server_auth::get_users(const network::request &)
    {
        LOG_TRACE("Get users request received");
        auto users = get_coco().get_module<coco_auth>().get_users();
        json::json users_json = json::json(json::json_type::array);
        for (auto &user : users)
        {
            auto j_itm = user.get().to_json();
            users_json.push_back(std::move(j_itm));
        }
        return std::make_unique<network::json_response>(std::move(users_json), network::status_code::ok);
    }

    std::unique_ptr<network::response> server_auth::create_user(const network::request &req)
    {
        LOG_TRACE("Create user request received");
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("username") || body["username"].get_type() != json::json_type::string || !body.contains("password") || body["password"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string username = body["username"];
        std::string password = body["password"];
        json::json personal_data = json::json(json::json_type::object);
        if (body.contains("personal_data") && body["personal_data"].get_type() == json::json_type::object)
            personal_data = body["personal_data"];

        try
        {
            [[maybe_unused]] auto &itm = get_coco().get_module<coco_auth>().create_user(username, password, std::move(personal_data));
            return std::make_unique<network::json_response>(json::json({{"token", get_coco().get_module<coco_auth>().get_token(username, password)}}), network::status_code::created);
        }
        catch (const std::invalid_argument &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::bad_request);
        }
    }

    void server_auth::on_ws_open(network::ws_server_session_base &ws)
    {
        LOG_TRACE("New connection");

        std::lock_guard<std::mutex> _(mtx);
        clients.emplace(&ws, "");
    }
    void server_auth::on_ws_message(network::ws_server_session_base &ws, const network::message &msg)
    {
        auto x = json::load(msg.get_payload());
        if (x.get_type() != json::json_type::object || !x.contains("msg_type") || x["msg_type"].get_type() != json::json_type::string)
        {
            ws.close();
            return;
        }

        std::string msg_type = x["msg_type"];
        if (msg_type == "login")
        {
            if (!x.contains("token") || x["token"].get_type() != json::json_type::string)
            {
                ws.close();
                return;
            }
            std::string token = x["token"];

            auto &auth = get_coco().get_module<coco_auth>();
            if (auth.is_valid_token(token))
            {
                LOG_DEBUG("User authenticated successfully");

                clients.at(&ws) = token;

                // Send user data
                auto usr = get_coco().get_db().get_module<auth_db>().get_user(token);
                json::json resp{{"msg_type", "login"}};
                if (usr.personal_data.size())
                    resp["personal_data"] = usr.personal_data;
                ws.send(resp.dump());

                // Send current state
                auto jc = get_coco().to_json();
                jc["msg_type"] = "coco";
                ws.send(jc.dump());
            }
            else
                ws.close();
        }
    }
    void server_auth::on_ws_close(network::ws_server_session_base &ws)
    {
        LOG_TRACE("Connection closed");
        std::lock_guard<std::mutex> _(mtx);
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void server_auth::on_ws_error(network::ws_server_session_base &ws, [[maybe_unused]] const std::error_code &ec)
    {
        LOG_TRACE("Connection error: " << ec.message());
        on_ws_close(ws);
    }

    void server_auth::broadcast(json::json &msg)
    {
        auto msg_str = msg.dump();
        for (auto client : clients)
            client.first->send(msg_str);
    }
} // namespace coco
