#include "coco_server.hpp"
#include "mongo_db.hpp"
#include "coco_api.hpp"
#include "logging.hpp"

namespace coco
{
#ifdef ENABLE_TRANSFORMER
    coco_server::coco_server(const std::string &host, unsigned short port, std::unique_ptr<coco::coco_db> db, const std::string &transformer_host, unsigned short transformer_port) : coco_core(db ? std::move(db) : std::make_unique<mongo_db>(), transformer_host, transformer_port), server(host, port)
#else
    coco_server::coco_server(const std::string &host, unsigned short port, std::unique_ptr<coco::coco_db> db) : coco_core(db ? std::move(db) : std::make_unique<mongo_db>()), server(host, port)
#endif
    {
        LOG_TRACE("OpenAPI: " + build_open_api().dump());
        LOG_TRACE("AsyncAPI: " + build_async_api().dump());

        init_routes();
    }

    void coco_server::init_routes()
    {
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&coco_server::assets, this, network::placeholders::request));
        add_route(network::Get, "^/open_api$", std::bind(&coco_server::open_api, this, network::placeholders::request));
        add_route(network::Get, "^/async_api$", std::bind(&coco_server::async_api, this, network::placeholders::request));

#ifdef ENABLE_AUTH
        add_route(network::Post, "^/login$", std::bind(&coco_server::login, this, network::placeholders::request));
        add_route(network::Post, "^/user$", std::bind(&coco_server::create_user, this, network::placeholders::request));
        add_route(network::Put, "^/user/.*$", std::bind(&coco_server::update_user, this, network::placeholders::request));
        add_route(network::Delete, "^/user/.*$", std::bind(&coco_server::delete_user, this, network::placeholders::request));
#endif

        add_route(network::Get, "^/types$", std::bind(&coco_server::get_types, this, network::placeholders::request));
        add_route(network::Get, "^/type(\\?.*)?$", std::bind(&coco_server::get_type, this, network::placeholders::request));
        add_route(network::Post, "^/type$", std::bind(&coco_server::create_type, this, network::placeholders::request));
        add_route(network::Put, "^/type/.*$", std::bind(&coco_server::update_type, this, network::placeholders::request));
        add_route(network::Delete, "^/type/.*$", std::bind(&coco_server::delete_type, this, network::placeholders::request));

        add_route(network::Get, "^/items(\\?.*)?$", std::bind(&coco_server::get_items, this, network::placeholders::request));
        add_route(network::Get, "^/item(\\?.*)?$", std::bind(&coco_server::get_item, this, network::placeholders::request));
        add_route(network::Post, "^/item$", std::bind(&coco_server::create_item, this, network::placeholders::request));
        add_route(network::Put, "^/item/.*$", std::bind(&coco_server::update_item, this, network::placeholders::request));
        add_route(network::Delete, "^/item/.*$", std::bind(&coco_server::delete_item, this, network::placeholders::request));

        add_route(network::Get, "^/data/.*$", std::bind(&coco_server::get_data, this, network::placeholders::request));
        add_route(network::Post, "^/data/.*$", std::bind(&coco_server::add_data, this, network::placeholders::request));

        add_route(network::Get, "^/reactive_rules$", std::bind(&coco_server::get_reactive_rules, this, network::placeholders::request));
        add_route(network::Post, "^/reactive_rule$", std::bind(&coco_server::create_reactive_rule, this, network::placeholders::request));
        add_route(network::Put, "^/reactive_rule/.*$", std::bind(&coco_server::update_reactive_rule, this, network::placeholders::request));
        add_route(network::Delete, "^/reactive_rule/.*$", std::bind(&coco_server::delete_reactive_rule, this, network::placeholders::request));

        add_route(network::Get, "^/deliberative_rules$", std::bind(&coco_server::get_deliberative_rules, this, network::placeholders::request));
        add_route(network::Post, "^/deliberative_rule$", std::bind(&coco_server::create_deliberative_rule, this, network::placeholders::request));
        add_route(network::Put, "^/deliberative_rule/.*$", std::bind(&coco_server::update_deliberative_rule, this, network::placeholders::request));
        add_route(network::Delete, "^/deliberative_rule/.*$", std::bind(&coco_server::delete_deliberative_rule, this, network::placeholders::request));

        add_ws_route("/coco").on_open(std::bind(&coco_server::on_ws_open, this, network::placeholders::request)).on_message(std::bind(&coco_server::on_ws_message, this, std::placeholders::_1, std::placeholders::_2)).on_close(std::bind(&coco_server::on_ws_close, this, network::placeholders::request)).on_error(std::bind(&coco_server::on_ws_error, this, network::placeholders::request, std::placeholders::_2));
    }

    std::unique_ptr<network::response> coco_server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/dist/index.html"); }
    std::unique_ptr<network::response> coco_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR "/dist" + target);
    }
    std::unique_ptr<network::response> coco_server::open_api([[maybe_unused]] const network::request &req)
    {
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res)
            return res;
#endif
        return std::make_unique<network::json_response>(build_open_api());
    }
    std::unique_ptr<network::response> coco_server::async_api([[maybe_unused]] const network::request &req)
    {
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res)
            return res;
#endif
        return std::make_unique<network::json_response>(build_async_api());
    }

#ifdef ENABLE_AUTH
    std::unique_ptr<network::response> coco_server::login(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.contains("username") || !body.contains("password"))
            return std::make_unique<network::json_response>(json::json{{"message", "Bad Request"}}, network::status_code::bad_request);
        if (auto usr = db->get_user(body["username"], body["password"]); usr)
            return std::make_unique<network::json_response>(json::json{{"token", usr.value().get().get_id()}});
        else
            return std::make_unique<network::json_response>(json::json{{"message", "Unauthorized"}}, network::status_code::unauthorized);
    }
    std::unique_ptr<network::response> coco_server::create_user(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("username") || body["username"].get_type() != json::json_type::string || !body.contains("password") || body["password"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string username = body["username"];
        std::string password = body["password"];
        json::json personal_data;
        json::json data;
        int role = roles::user;
        if (body.contains("data") && body["data"].contains("role"))
        {
            if (body["data"]["role"].get_type() != json::json_type::number)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            role = body["data"]["role"];
            switch (role)
            {
            case roles::admin: // Admins can only be created by other admins
                if (auto res = authorize(req, {roles::admin}); res)
                    return res;
                break;
            case roles::coordinator: // Coordinators can be created by admins and coordinators
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res)
                    return res;
                break;
            }
            data = body["data"];
        }
        if (!data.contains("role"))
            data["role"] = role;
        if (body.contains("personal_data"))
            personal_data = body["personal_data"];
        return std::make_unique<network::json_response>(json::json{{"token", coco_core::create_user(username, password, std::move(personal_data), std::move(data)).get_id()}});
    }
    std::unique_ptr<network::response> coco_server::update_user(const network::request &req)
    {
        auto id = req.get_target().substr(6);
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the user is an admin or coordinator, they can only update themselves
            return res;

        if (!db->has_item(id))
            return std::make_unique<network::json_response>(json::json({{"message", "User not found"}}), network::status_code::not_found);

        auto &usr = db->get_item(id);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.contains("username"))
        {
            if (body["username"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_user_username(usr, body["username"]);
        }
        if (body.contains("password"))
        {
            if (body["password"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_user_password(usr, body["password"]);
        }
        if (body.contains("personal_data"))
        {
            if (body["personal_data"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            json::json personal_data = body["personal_data"];
            set_user_personal_data(usr, std::move(personal_data));
        }
        if (body.contains("data") && body["data"].contains("role"))
        {
            if (body["data"]["role"].get_type() != json::json_type::number)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            int role = body["data"]["role"];
            switch (role)
            {
            case roles::admin: // Admins can only be created by other admins
                if (auto res = authorize(req, {roles::admin}); res)
                    return res;
                break;
            case roles::coordinator: // Coordinators can be created by admins and coordinators
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res)
                    return res;
                break;
            }
            json::json props = body["data"];
            set_item_properties(usr, std::move(props));
        }
        return std::make_unique<network::json_response>(to_json(usr));
    }
    std::unique_ptr<network::response> coco_server::delete_user(const network::request &req)
    {
        auto id = req.get_target().substr(6);
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the user is an admin or coordinator, they can only delete themselves
            return res;

        if (!db->has_item(id))
            return std::make_unique<network::json_response>(json::json({{"message", "User not found"}}), network::status_code::not_found);

        db->delete_user(db->get_item(id));
        return std::make_unique<network::json_response>(json::json({{"message", "User deleted"}}));
    }
#endif

    std::unique_ptr<network::response> coco_server::get_types([[maybe_unused]] const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all types
            return res;
#endif
        json::json sts(json::json_type::array);
        for (auto &tp : coco_core::get_types())
            sts.push_back(to_json(tp));
        return std::make_unique<network::json_response>(std::move(sts));
    }
    std::unique_ptr<network::response> coco_server::get_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        if (req.get_target().find('?') == std::string::npos)
            try // get type by id in the path
            {
                auto &tp = coco_core::get_type(req.get_target().substr(6));
#ifdef ENABLE_AUTH
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res)
                {
                    auto token = req.get_headers().at("authorization").substr(7);
                    if (db->has_item(token) && db->get_item(token).get_type().is_assignable_from(tp))
                        return std::make_unique<network::json_response>(to_json(tp));
                    else
                        return res;
                }
#endif
                return std::make_unique<network::json_response>(to_json(tp));
            }
            catch (const std::exception &)
            {
#ifdef ENABLE_AUTH
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all types
                    return res;
#endif
                return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
            }
        else
        { // get type by name
            auto params = network::parse_query(req.get_target().substr(req.get_target().find('?') + 1));
            if (!params.count("name"))
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            try
            {
                auto &tp = coco_core::get_type_by_name(params.at("name"));
#ifdef ENABLE_AUTH
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res)
                {
                    auto token = req.get_headers().at("authorization").substr(7);
                    if (db->has_item(token) && db->get_item(token).get_type().is_assignable_from(tp))
                        return std::make_unique<network::json_response>(to_json(tp));
                    else
                        return res;
                }
#endif
                return std::make_unique<network::json_response>(to_json(tp));
            }
            catch (const std::exception &)
            {
#ifdef ENABLE_AUTH
                if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all types
                    return res;
#endif
                return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
            }
        }
    }
    std::unique_ptr<network::response> coco_server::create_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can create types
            return res;
#endif
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        std::string name = body["name"];
        std::string description = body.contains("description") ? body["description"] : "";
        json::json props = body.contains("properties") ? body["properties"] : json::json();
        auto &tp = coco_core::create_type(name, description, std::move(props));

        if (body.contains("parents") && body["parents"].get_type() == json::json_type::array)
        {
            std::vector<std::reference_wrapper<const type>> parents;
            for (auto &p : body["parents"].as_array())
            {
                if (p.get_type() != json::json_type::string)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                try
                {
                    auto &tp = coco_core::get_type(p);
                    parents.emplace_back(tp);
                }
                catch (const std::exception &)
                {
                    return std::make_unique<network::json_response>(json::json({{"message", "Parent type not found"}}), network::status_code::not_found);
                }
            }
            set_type_parents(tp, std::move(parents));
        }

        if (body.contains("static_properties"))
        {
            std::vector<std::unique_ptr<property>> static_properties;
            if (body["static_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["static_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(tp, p.first, p.second);
                static_properties.emplace_back(std::move(prop));
            }
            set_type_static_properties(tp, std::move(static_properties));
        }

        if (body.contains("dynamic_properties"))
        {
            std::vector<std::unique_ptr<property>> dynamic_properties;
            if (body["dynamic_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["dynamic_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(tp, p.first, p.second);
                dynamic_properties.emplace_back(std::move(prop));
            }
            set_type_dynamic_properties(tp, std::move(dynamic_properties));
        }

        return std::make_unique<network::json_response>(to_json(tp));
    }
    std::unique_ptr<network::response> coco_server::update_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can update types
            return res;
#endif
        auto id = req.get_target().substr(6);
        type *tp;
        try
        {
            tp = &coco_core::get_type(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.contains("name"))
        {
            if (body["name"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_type_name(*tp, body["name"]);
        }
        if (body.contains("description"))
        {
            if (body["description"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_type_description(*tp, body["description"]);
        }
        if (body.contains("properties"))
        {
            if (body["properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            json::json props = body["properties"];
            set_type_properties(*tp, std::move(props));
        }
        if (body.contains("parents"))
        {
            if (body["parents"].get_type() != json::json_type::array)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["parents"].as_array())
            {
                if (p.get_type() != json::json_type::string)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                if (!db->has_type(p))
                    return std::make_unique<network::json_response>(json::json({{"message", "Parent type not found"}}), network::status_code::not_found);
            }

            std::vector<std::reference_wrapper<const type>> parents;
            for (auto &p : body["parents"].as_array())
                parents.emplace_back(coco_core::get_type(p));
            set_type_parents(*tp, std::move(parents));
        }
        if (body.contains("static_properties"))
        {
            if (body["static_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["static_properties"].as_object())
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

            std::vector<std::unique_ptr<property>> static_properties;
            for (auto &[name, prop] : body["static_properties"].as_object())
                static_properties.emplace_back(make_property(*tp, name, prop));
            set_type_static_properties(*tp, std::move(static_properties));
        }
        if (body.contains("dynamic_properties"))
        {
            if (body["dynamic_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["dynamic_properties"].as_object())
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

            std::vector<std::unique_ptr<property>> dynamic_properties;
            for (auto &[name, prop] : body["dynamic_properties"].as_object())
                dynamic_properties.emplace_back(make_property(*tp, name, prop));
            set_type_dynamic_properties(*tp, std::move(dynamic_properties));
        }
        return std::make_unique<network::json_response>(to_json(*tp));
    }
    std::unique_ptr<network::response> coco_server::delete_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can delete types
            return res;
#endif
        auto id = req.get_target().substr(6);
        try
        {
            type &tp = coco_core::get_type(id);
            coco_core::delete_type(tp);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_items(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all items
            return res;
#endif
        json::json ss(json::json_type::array);
        if (req.get_target().find('?') != std::string::npos)
        {
            auto params = network::parse_query(req.get_target().substr(req.get_target().find('?') + 1));
            if (!params.count("type_id") && !params.count("type_name"))
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            type *tp;
            try
            {
                if (params.count("type_id"))
                    tp = &coco_core::get_type(params.at("type_id"));
                else
                    tp = &coco_core::get_type_by_name(params.at("type_name"));
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
            }
            for (auto &itm : coco_core::get_items_by_type(*tp))
                ss.push_back(to_json(itm));
        }
        else
            for (auto &itm : coco_core::get_items())
                ss.push_back(to_json(itm));
        return std::make_unique<network::json_response>(std::move(ss));
    }
    std::unique_ptr<network::response> coco_server::get_item(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = req.get_target().substr(6);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the client is an admin or coordinator, they can only get themselves
            return res;
#endif
        try
        {
            auto &itm = coco_core::get_item(id);
            return std::make_unique<network::json_response>(to_json(itm));
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::create_item(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can create items
            return res;
#endif
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("type") || body["type"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string type = body["type"];
        coco::type *tp;
        try
        {
            tp = &coco_core::get_type(type);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
        try
        {
            json::json props = body.contains("properties") ? body["properties"] : json::json();
            auto &s = coco_core::create_item(*tp, std::move(props));
            return std::make_unique<network::json_response>(to_json(s));
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::update_item(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = req.get_target().substr(6);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the client is an admin or coordinator, they can only update themselves
            return res;
#endif
        item *itm;
        try
        {
            itm = &coco_core::get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.contains("properties"))
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        if (body["properties"].get_type() != json::json_type::object)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        json::json props = body["properties"];
        set_item_properties(*itm, std::move(props));
        return std::make_unique<network::json_response>(to_json(*itm));
    }
    std::unique_ptr<network::response> coco_server::delete_item(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = req.get_target().substr(6);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the client is an admin or coordinator, they can only delete themselves
            return res;
#endif
        try
        {
            item &itm = coco_core::get_item(id);
            coco_core::delete_item(itm);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_data(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = req.get_target().substr(6);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the client is an admin or coordinator, they can only get their own data
            return res;
#endif
        std::map<std::string, std::string> params;
        if (id.find('?') != std::string::npos)
        {
            params = network::parse_query(id.substr(id.find('?') + 1));
            id = id.substr(0, id.find('?'));
        }
        item *itm;
        try
        {
            itm = &coco_core::get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        std::chrono::system_clock::time_point to = params.count("to") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("to"))}) : std::chrono::system_clock::time_point();
        std::chrono::system_clock::time_point from = params.count("from") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("from"))}) : to - std::chrono::hours{24 * 30};
        return std::make_unique<network::json_response>(coco_core::get_data(*itm, from, to));
    }
    std::unique_ptr<network::response> coco_server::add_data(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = req.get_target().substr(6);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}, {id}); res) // Unless the client is an admin or coordinator, they can only add data to their own items
            return res;
#endif
        item *itm;
        try
        {
            itm = &coco_core::get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        coco_core::add_data(*itm, body);
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    std::unique_ptr<network::response> coco_server::get_reactive_rules([[maybe_unused]] const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all reactive rules
            return res;
#endif
        json::json rls(json::json_type::array);
        for (auto &rl : coco_core::get_reactive_rules())
            rls.push_back(to_json(rl));
        return std::make_unique<network::json_response>(std::move(rls));
    }
    std::unique_ptr<network::response> coco_server::create_reactive_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can create reactive rules
            return res;
#endif
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("content") || body["content"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string content = body["content"];
        try
        {
            auto &rl = coco_core::create_reactive_rule(name, content);
            return std::make_unique<network::json_response>(to_json(rl));
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::update_reactive_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can update reactive rules
            return res;
#endif
        auto id = req.get_target().substr(15);
        rule *rl;
        try
        {
            rl = &coco_core::get_reactive_rule(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Reactive rule not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.contains("name"))
        {
            if (body["name"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_reactive_rule_name(*rl, body["name"]);
        }
        if (body.contains("content"))
        {
            if (body["content"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_reactive_rule_content(*rl, body["content"]);
        }
        return std::make_unique<network::json_response>(to_json(*rl));
    }
    std::unique_ptr<network::response> coco_server::delete_reactive_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can delete reactive rules
            return res;
#endif
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        try
        {
            rule &rl = coco_core::get_reactive_rule(id);
            coco_core::delete_reactive_rule(rl);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Reactive rule not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_deliberative_rules([[maybe_unused]] const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin, roles::coordinator}); res) // Only admins and coordinators can get all deliberative rules
            return res;
#endif
        json::json rls(json::json_type::array);
        for (auto &rl : coco_core::get_deliberative_rules())
            rls.push_back(to_json(rl));
        return std::make_unique<network::json_response>(std::move(rls));
    }
    std::unique_ptr<network::response> coco_server::create_deliberative_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("content") || body["content"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string content = body["content"];
        try
        {
            auto &rl = coco_core::create_deliberative_rule(name, content);
            return std::make_unique<network::json_response>(to_json(rl));
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::update_deliberative_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can update deliberative rules
            return res;
#endif
        auto id = req.get_target().substr(19);
        rule *rl;
        try
        {
            rl = &coco_core::get_deliberative_rule(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Deliberative rule not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.contains("name"))
        {
            if (body["name"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_deliberative_rule_name(*rl, body["name"]);
        }
        if (body.contains("content"))
        {
            if (body["content"].get_type() != json::json_type::string)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            set_deliberative_rule_content(*rl, body["content"]);
        }
        return std::make_unique<network::json_response>(to_json(*rl));
    }
    std::unique_ptr<network::response> coco_server::delete_deliberative_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        if (auto res = authorize(req, {roles::admin}); res) // Only admins can delete deliberative rules
            return res;
#endif
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        try
        {
            rule &rl = coco_core::get_deliberative_rule(id);
            coco_core::delete_deliberative_rule(rl);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Deliberative rule not found"}}), network::status_code::not_found);
        }
    }

#ifdef ENABLE_AUTH
    std::unique_ptr<network::response> coco_server::authorize(const network::request &req, const std::set<int> &roles, std::set<std::string> exceptions)
    {
        if (auto auth = req.get_headers().find("authorization"); auth != req.get_headers().end())
            if (auth->second.size() > 7 && auth->second.substr(0, 7) == "Bearer ")
            {
                auto token = auth->second.substr(7);
                if (exceptions.count(token))
                    return nullptr; // token is valid
                if (!db->has_item(token))
                    return std::make_unique<network::json_response>(json::json{{"message", "Unauthorized"}}, network::status_code::unauthorized);
                auto &itm = db->get_item(token);
                if (!roles.count(static_cast<int>(itm.get_properties()["role"])))
                    return std::make_unique<network::json_response>(json::json{{"message", "Unauthorized"}}, network::status_code::unauthorized);
                return nullptr; // token is valid
            }
        return std::make_unique<network::json_response>(json::json{{"message", "Unauthorized"}}, network::status_code::unauthorized);
    }
#endif

    void coco_server::on_ws_open(network::ws_session &ws)
    {
        LOG_TRACE("New connection from " << ws.remote_endpoint());
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        clients.emplace(&ws, "");
#else
        clients.insert(&ws);

        // we send the types
        ws.send(make_types_message(*this).dump());

        // we send the items
        ws.send(make_items_message(*this).dump());

        // we send the reactive rules
        ws.send(make_reactive_rules_message(*this).dump());

        // we send the deliberative rules
        ws.send(make_deliberative_rules_message(*this).dump());

        // we send the solvers
        ws.send(make_solvers_message(*this).dump());

        // we send the executors
        for (const auto &cc_exec : get_solvers())
        {
            ws.send(make_solver_state_message(cc_exec.get()).dump());
            ws.send(make_solver_graph_message(cc_exec.get().get_solver().get_graph()).dump());
        }
#endif
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void coco_server::on_ws_message(network::ws_session &ws, const std::string &msg)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto x = json::load(msg);

        if (x.get_type() != json::json_type::object || !x.contains("type") || x["type"].get_type() != json::json_type::string)
        {
            ws.close();
            return;
        }

        std::string type = x["type"];

#ifdef ENABLE_AUTH
        if (type == "login")
        {
            if (x.get_type() != json::json_type::object || !x.contains("token") || x["token"].get_type() != json::json_type::string)
            {
                ws.close();
                return;
            }
            std::string token = x["token"];
            if (db->has_item(token))
            { // if the token is an item, we send some information about it
                clients.at(&ws) = token;
                devices[token].emplace(&ws);

                auto &itm = db->get_item(token);
                // we send the (user) type
                ws.send(make_user_type_message(itm.get_type()).dump());
                // we send the (user) item
                ws.send(make_user_message(itm).dump());

                int role = static_cast<int>(itm.get_properties()["role"]);
                if (role == roles::admin || role == roles::coordinator)
                {
                    if (auto it = users.find(token); it == users.end())
                        users.emplace(token, role);

                    // we send the types
                    ws.send(make_types_message(*this).dump());

                    // we send the items
                    ws.send(make_items_message(*this).dump());

                    if (role == roles::admin)
                    {
                        // we send the reactive rules
                        ws.send(make_reactive_rules_message(*this).dump());

                        // we send the deliberative rules
                        ws.send(make_deliberative_rules_message(*this).dump());

                        // we send the solvers
                        ws.send(make_solvers_message(*this).dump());

                        // we send the executors
                        for (const auto &cc_exec : get_solvers())
                        {
                            ws.send(make_solver_state_message(cc_exec.get()).dump());
                            ws.send(make_solver_graph_message(cc_exec.get().get_solver().get_graph()).dump());
                        }
                    }
                }
            }
            else
            {
                ws.close();
                return;
            }
        }
#endif
    }
    void coco_server::on_ws_close(network::ws_session &ws)
    {
        LOG_TRACE("Connection closed with " << ws.remote_endpoint());
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        auto token = clients.at(&ws);
        if (devices.count(token))
        {
            devices.at(token).erase(&ws);
            if (auto it = users.find(token); it != users.end())
                users.erase(it);
        }
#endif
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void coco_server::on_ws_error(network::ws_session &ws, [[maybe_unused]] const std::error_code &ec)
    {
        LOG_TRACE("Connection error with " << ws.remote_endpoint() << ": " << ec.message());
        on_ws_close(ws);
    }

    void coco_server::new_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_type_message(tp), {roles::admin, roles::coordinator});
#else
        broadcast(make_new_type_message(tp));
#endif
    }
    void coco_server::updated_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_updated_type_message(tp), {roles::admin, roles::coordinator});
#else
        broadcast(make_updated_type_message(tp));
#endif
    }
    void coco_server::deleted_type(const std::string &tp_id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_deleted_type_message(tp_id), {roles::admin, roles::coordinator});
#else
        broadcast(make_deleted_type_message(tp_id));
#endif
    }

    void coco_server::new_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_item_message(itm), {roles::admin, roles::coordinator});
#else
        broadcast(make_new_item_message(itm));
#endif
    }
    void coco_server::updated_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_updated_item_message(itm), {roles::admin, roles::coordinator});
        if (auto it = devices.find(itm.get_id()); it != devices.end()) // if the device is connected, we send the update to it
            for (auto &ws : it->second)
                ws->send(make_updated_item_message(itm).dump());
#else
        broadcast(make_updated_item_message(itm));
#endif
    }
    void coco_server::deleted_item(const std::string &itm_id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_deleted_item_message(itm_id), {roles::admin, roles::coordinator});
        if (auto it = devices.find(itm_id); it != devices.end()) // if the device is connected, we send the update to it
            for (auto &ws : it->second)
                ws->send(make_deleted_item_message(itm_id).dump());
#else
        broadcast(make_deleted_item_message(itm_id));
#endif
    }

    void coco_server::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_data_message(itm, data, timestamp), {roles::admin, roles::coordinator});
        if (auto it = devices.find(itm.get_id()); it != devices.end()) // if the device is connected, we send the update to it
            for (auto &ws : it->second)
                ws->send(make_new_data_message(itm, data, timestamp).dump());
#else
        broadcast(make_new_data_message(itm, data, timestamp));
#endif
    }

    void coco_server::new_solver(const coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_solver_message(exec), {roles::admin});
#else
        broadcast(make_new_solver_message(exec));
#endif
    }
    void coco_server::deleted_solver(const uintptr_t id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(ratio::executor::make_deleted_solver_message(id), {roles::admin});
#else
        broadcast(ratio::executor::make_deleted_solver_message(id));
#endif
    }

    void coco_server::new_reactive_rule(const rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_reactive_rule_message(r), {roles::admin});
#else
        broadcast(make_new_reactive_rule_message(r));
#endif
    }
    void coco_server::new_deliberative_rule(const rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_new_deliberative_rule_message(r), {roles::admin});
#else
        broadcast(make_new_deliberative_rule_message(r));
#endif
    }

    void coco_server::state_changed(const coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_solver_state_message(exec), {roles::admin});
#else
        broadcast(make_solver_state_message(exec));
#endif
    }

    void coco_server::flaw_created(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_flaw_created_message(f), {roles::admin});
#else
        broadcast(make_flaw_created_message(f));
#endif
    }
    void coco_server::flaw_state_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_flaw_state_changed_message(f), {roles::admin});
#else
        broadcast(make_flaw_state_changed_message(f));
#endif
    }
    void coco_server::flaw_cost_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_flaw_cost_changed_message(f), {roles::admin});
#else
        broadcast(make_flaw_cost_changed_message(f));
#endif
    }
    void coco_server::flaw_position_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_flaw_position_changed_message(f), {roles::admin});
#else
        broadcast(make_flaw_position_changed_message(f));
#endif
    }
    void coco_server::current_flaw(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_current_flaw_message(f), {roles::admin});
#else
        broadcast(make_current_flaw_message(f));
#endif
    }

    void coco_server::resolver_created(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_resolver_created_message(r), {roles::admin});
#else
        broadcast(make_resolver_created_message(r));
#endif
    }
    void coco_server::resolver_state_changed(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_resolver_state_changed_message(r), {roles::admin});
#else
        broadcast(make_resolver_state_changed_message(r));
#endif
    }
    void coco_server::current_resolver(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_current_resolver_message(r), {roles::admin});
#else
        broadcast(make_current_resolver_message(r));
#endif
    }

    void coco_server::causal_link_added(const coco_executor &, const ratio::flaw &f, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_causal_link_added_message(f, r), {roles::admin});
#else
        broadcast(make_causal_link_added_message(f, r));
#endif
    }

    void coco_server::executor_state_changed(const coco_executor &exec, ratio::executor::executor_state)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_solver_execution_state_changed_message(exec), {roles::admin});
#else
        broadcast(make_solver_execution_state_changed_message(exec));
#endif
    }

    void coco_server::tick(const coco_executor &exec, const utils::rational &)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
#ifdef ENABLE_AUTH
        broadcast(make_tick_message(exec), {roles::admin});
#else
        broadcast(make_tick_message(exec));
#endif
    }

    [[nodiscard]] json::json build_schemas() noexcept
    {
        json::json components;
        for (const auto &s : coco_schemas.as_object())
            components[s.first] = s.second;
        for (const auto &s : ratio::solver_schemas.as_object())
            components[s.first] = s.second;
        for (const auto &s : ratio::executor::executor_schemas.as_object())
            components[s.first] = s.second;
        return components;
    }
    [[nodiscard]] json::json build_messages() noexcept
    {
        json::json messages;
        for (const auto &m : coco_messages.as_object())
            messages[m.first] = m.second;
        for (const auto &m : ratio::solver_messages.as_object())
            messages[m.first] = m.second;
        for (const auto &m : ratio::executor::executor_messages.as_object())
            messages[m.first] = m.second;
        return messages;
    }
    [[nodiscard]] json::json build_paths() noexcept
    {
        json::json paths{{"/",
                          {{"get",
                            {{"summary", "Index"},
                             {"description", "Index page"},
                             {"responses",
                              {{"200", {{"description", "Index page"}}}}}}}}},
                         {"/assets/{file}",
                          {{"get",
                            {{"summary", "Assets"},
                             {"description", "Assets"},
                             {"parameters", std::vector<json::json>{{{"name", "file"}, {"in", "path"}, {"required", true}, {"schema", {{"type", "string"}}}}}},
                             {"responses",
                              {{"200", {{"description", "Index page"}}}}}}}}},
                         {"/open_api",
                          {{"get",
                            {{"summary", "Retrieve OpenAPI Specification"},
                             {"description", "Endpoint to fetch the OpenAPI Specification document"},
#ifdef ENABLE_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"200", {{"description", "Successful response with OpenAPI Specification document"}}}}}}}}},
                         {"/async_api",
                          {{"get",
                            {{"summary", "Retrieve AsyncAPI Specification"},
                             {"description", "Endpoint to fetch the AsyncAPI Specification document"},
#ifdef ENABLE_AUTH
                             {"security", std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}}},
#endif
                             {"responses",
                              {{"200", {{"description", "Successful response with AsyncAPI Specification document"}}}}}}}}}};
        for (const auto &p : coco_paths.as_object())
            paths[p.first] = p.second;
        return paths;
    }
    [[nodiscard]] json::json build_open_api() noexcept
    {
        json::json open_api{
            {"openapi", "3.1.0"},
            {"info",
             {{"title", "CoCo API"},
              {"description", "The combined deduCtiOn and abduCtiOn (CoCo) API"},
              {"version", "1.0"}}},
            {"components", {
#ifdef ENABLE_AUTH
                               {"securitySchemes", {"bearerAuth", {{"type", "http"}, {"scheme", "bearer"}}}},
#endif
                               {"schemas", build_schemas()}}},
            {"paths", build_paths()},
            {"servers", std::vector<json::json>{{"url", "http://" SERVER_HOST ":" + std::to_string(SERVER_PORT)}}}};
        return open_api;
    }
    [[nodiscard]] json::json build_async_api() noexcept
    {
        json::json async_api{
            {"asyncapi", "3.0.0"},
            {"info",
             {{"title", "CoCo API"},
              {"description", "The combined deduCtiOn and abduCtiOn (CoCo) WebSocket API"},
              {"version", "1.0"}}},
            {"servers", {"coco", {{"host", SERVER_HOST ":" + std::to_string(SERVER_PORT)}, {"pathname", "/coco"}, {"protocol", "ws"}}}},
            {"channels", {{"coco", {{"address", "/"}}}}},
            {"components", {
#ifdef ENABLE_AUTH
                               {"securitySchemes", {"bearerAuth", {{"type", "http"}, {"scheme", "bearer"}}}},
#endif
                               {"messages", build_messages()},
                               {"schemas", build_schemas()}}}};
        return async_api;
    }
} // namespace coco