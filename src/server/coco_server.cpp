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

#ifdef ENABLE_AUTH
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request), true);
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&coco_server::assets, this, network::placeholders::request), true);
#else
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&coco_server::assets, this, network::placeholders::request));
#endif
        add_route(network::Get, "^/open_api$", std::bind(&coco_server::open_api, this, network::placeholders::request));
        add_route(network::Get, "^/async_api$", std::bind(&coco_server::async_api, this, network::placeholders::request));

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
    std::unique_ptr<network::response> coco_server::open_api(const network::request &) { return std::make_unique<network::json_response>(build_open_api()); }
    std::unique_ptr<network::response> coco_server::async_api(const network::request &) { return std::make_unique<network::json_response>(build_async_api()); }

    std::unique_ptr<network::response> coco_server::get_types(const network::request &)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        json::json sts(json::json_type::array);
        for (auto &tp : coco_core::get_types())
            sts.push_back(to_json(tp));
        return std::make_unique<network::json_response>(std::move(sts));
    }
    std::unique_ptr<network::response> coco_server::get_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        if (req.get_target().find('?') == std::string::npos)
        {
            auto id = req.get_target().substr(6);
            try
            {
                auto &tp = coco_core::get_type(id);
                return std::make_unique<network::json_response>(to_json(tp));
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
            }
        }
        else
        {
            auto params = network::parse_query(req.get_target().substr(req.get_target().find('?') + 1));
            if (!params.count("name"))
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            try
            {
                auto &tp = coco_core::get_type_by_name(params.at("name"));
                return std::make_unique<network::json_response>(to_json(tp));
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
            }
        }
    }
    std::unique_ptr<network::response> coco_server::create_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string description = body.contains("description") ? body["description"] : "";
        std::vector<std::reference_wrapper<const type>> parents;
        if (body.contains("parents") && body["parents"].get_type() == json::json_type::array)
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
        std::vector<std::unique_ptr<property>> static_properties;
        if (body.contains("static_properties"))
        {
            if (body["static_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["static_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(*this, p.first, p.second);
                static_properties.emplace_back(std::move(prop));
            }
        }
        std::vector<std::unique_ptr<property>> dynamic_properties;
        if (body.contains("dynamic_properties"))
        {
            if (body["dynamic_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["dynamic_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(*this, p.first, p.second);
                dynamic_properties.emplace_back(std::move(prop));
            }
        }
        try
        {
            auto &tp = coco_core::create_type(name, description, std::move(parents), std::move(static_properties), std::move(dynamic_properties));
            return std::make_unique<network::json_response>(to_json(tp));
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::update_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::string id = req.get_target().substr(7);
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
        if (body.contains("parents"))
        {
            if (body["parents"].get_type() != json::json_type::array)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : tp->get_parents())
                remove_parent(*tp, p.second.get());
            for (auto &p : body["parents"].as_array())
            {
                if (p.get_type() != json::json_type::string)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                try
                {
                    auto &parent = coco_core::get_type(p);
                    add_parent(*tp, parent);
                }
                catch (const std::exception &)
                {
                    return std::make_unique<network::json_response>(json::json({{"message", "Parent type not found"}}), network::status_code::not_found);
                }
            }
        }
        if (body.contains("static_properties"))
        {
            if (body["static_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : tp->get_static_properties())
                remove_static_property(*tp, *p.second);
            for (auto &p : body["static_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(*this, p.first, p.second);
                add_static_property(*tp, std::move(prop));
            }
        }
        if (body.contains("dynamic_properties"))
        {
            if (body["dynamic_properties"].get_type() != json::json_type::object)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : tp->get_dynamic_properties())
                remove_dynamic_property(*tp, *p.second);
            for (auto &p : body["dynamic_properties"].as_object())
            {
                if (p.second.get_type() != json::json_type::object)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                auto prop = make_property(*this, p.first, p.second);
                add_dynamic_property(*tp, std::move(prop));
            }
        }
        return std::make_unique<network::json_response>(to_json(*tp));
    }
    std::unique_ptr<network::response> coco_server::delete_type(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        type *tp;
        try
        {
            tp = &coco_core::get_type(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
        coco_core::delete_type(*tp);
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    std::unique_ptr<network::response> coco_server::get_items(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
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
            auto &s = coco_core::create_item(*tp, body.contains("properties") ? body["properties"] : json::json());
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
        std::string id = req.get_target().substr(7);
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
        set_item_properties(*itm, body["properties"]);
        return std::make_unique<network::json_response>(to_json(*itm));
    }
    std::unique_ptr<network::response> coco_server::delete_item(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        item *itm;
        try
        {
            itm = &coco_core::get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        coco_core::delete_item(*itm);
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    std::unique_ptr<network::response> coco_server::get_data(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::string id = req.get_target().substr(6);
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
        std::string id = req.get_target().substr(6);
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

    std::unique_ptr<network::response> coco_server::get_reactive_rules(const network::request &)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        json::json rls(json::json_type::array);
        for (auto &rl : coco_core::get_reactive_rules())
            rls.push_back(to_json(rl));
        return std::make_unique<network::json_response>(std::move(rls));
    }
    std::unique_ptr<network::response> coco_server::create_reactive_rule(const network::request &req)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
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
        std::string id = req.get_target().substr(15);
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
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        rule *rl;
        try
        {
            rl = &coco_core::get_reactive_rule(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Reactive rule not found"}}), network::status_code::not_found);
        }
        coco_core::delete_reactive_rule(*rl);
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    std::unique_ptr<network::response> coco_server::get_deliberative_rules(const network::request &)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
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
        std::string id = req.get_target().substr(18);
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
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("id") || body["id"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string id = body["id"];
        rule *rl;
        try
        {
            rl = &coco_core::get_deliberative_rule(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Deliberative rule not found"}}), network::status_code::not_found);
        }
        coco_core::delete_deliberative_rule(*rl);
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    void coco_server::on_ws_open(network::ws_session &ws)
    {
        LOG_TRACE("New connection from " << ws.remote_endpoint());
        std::lock_guard<std::recursive_mutex> _(mtx);
        clients.insert(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));

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
    }
    void coco_server::on_ws_message(network::ws_session &ws, const std::string &msg)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto x = json::load(msg);
        if (x.get_type() != json::json_type::object || !x.contains("type"))
        {
            ws.close();
            return;
        }
    }
    void coco_server::on_ws_close(network::ws_session &ws)
    {
        LOG_TRACE("Connection closed with " << ws.remote_endpoint());
        std::lock_guard<std::recursive_mutex> _(mtx);
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }
    void coco_server::on_ws_error(network::ws_session &ws, const std::error_code &)
    {
        LOG_TRACE("Connection error with " << ws.remote_endpoint());
        std::lock_guard<std::recursive_mutex> _(mtx);
        clients.erase(&ws);
        LOG_DEBUG("Connected clients: " + std::to_string(clients.size()));
    }

    void coco_server::new_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_type_message(tp));
    }
    void coco_server::updated_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_updated_type_message(tp));
    }
    void coco_server::deleted_type(const std::string &tp_id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_deleted_type_message(tp_id));
    }

    void coco_server::new_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_item_message(itm));
    }
    void coco_server::updated_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_updated_item_message(itm));
    }
    void coco_server::deleted_item(const std::string &itm_id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_deleted_item_message(itm_id));
    }

    void coco_server::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_data_message(itm, data, timestamp));
    }

    void coco_server::new_solver(const coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_solver_message(exec));
    }
    void coco_server::deleted_solver(const uintptr_t id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(ratio::executor::make_deleted_solver_message(id));
    }

    void coco_server::new_reactive_rule(const rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_reactive_rule_message(r));
    }
    void coco_server::new_deliberative_rule(const rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_new_deliberative_rule_message(r));
    }

    void coco_server::state_changed(const coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_solver_state_message(exec));
    }

    void coco_server::flaw_created(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_flaw_created_message(f));
    }
    void coco_server::flaw_state_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_flaw_state_changed_message(f));
    }
    void coco_server::flaw_cost_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_flaw_cost_changed_message(f));
    }
    void coco_server::flaw_position_changed(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_flaw_position_changed_message(f));
    }
    void coco_server::current_flaw(const coco_executor &, const ratio::flaw &f)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_current_flaw_message(f));
    }

    void coco_server::resolver_created(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_resolver_created_message(r));
    }
    void coco_server::resolver_state_changed(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_resolver_state_changed_message(r));
    }
    void coco_server::current_resolver(const coco_executor &, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_current_resolver_message(r));
    }

    void coco_server::causal_link_added(const coco_executor &, const ratio::flaw &f, const ratio::resolver &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_causal_link_added_message(f, r));
    }

    void coco_server::executor_state_changed(const coco_executor &exec, ratio::executor::executor_state)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_solver_execution_state_changed_message(exec));
    }

    void coco_server::tick(const coco_executor &exec, const utils::rational &)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        broadcast(make_tick_message(exec));
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
#ifdef ENABLE_AUTH
        components["securitySchemes"] = {{"bearerAuth", {{"type", "http"}, {"scheme", "bearer"}}}};
#endif
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
                             {"responses",
                              {{"200", {{"description", "Successful response with OpenAPI Specification document"}}}}}}}}},
                         {"/async_api",
                          {{"get",
                            {{"summary", "Retrieve AsyncAPI Specification"},
                             {"description", "Endpoint to fetch the AsyncAPI Specification document"},
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
            {"components", {"schemas", build_schemas()}},
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
            {"components",
             {{"messages", build_messages()},
              {"schemas", build_schemas()}}}};
        return async_api;
    }
} // namespace coco