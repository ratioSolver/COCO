#include "coco_server.hpp"
#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"

namespace coco
{
    server_module::server_module(coco_server &srv) noexcept : srv(srv) {}
    coco &server_module::get_coco() noexcept { return srv.get_coco(); }

#ifdef ENABLE_SSL
    coco_server::coco_server(coco &cc, std::string_view host, unsigned short port) : coco_module(cc), listener(cc), ssl_server(host, port)
#else
    coco_server::coco_server(coco &cc, std::string_view host, unsigned short port) : coco_module(cc), listener(cc), server(host, port)
#endif
    {
        add_route(network::Get, "^/$", std::bind(&coco_server::index, this, network::placeholders::request));
        add_route(network::Get, "^(/assets/.+)|/.+\\.ico|/.+\\.png", std::bind(&coco_server::assets, this, network::placeholders::request));

        add_route(network::Get, "^/types$", std::bind(&coco_server::get_types, this, network::placeholders::request));
        add_route(network::Get, "^/types/.*$", std::bind(&coco_server::get_type, this, network::placeholders::request));
        add_route(network::Post, "^/types$", std::bind(&coco_server::create_type, this, network::placeholders::request));
        add_route(network::Delete, "^/types/.*$", std::bind(&coco_server::delete_type, this, network::placeholders::request));

        add_route(network::Get, "^/items$", std::bind(&coco_server::get_items, this, network::placeholders::request));
        add_route(network::Get, "^/items/.*$", std::bind(&coco_server::get_item, this, network::placeholders::request));
        add_route(network::Post, "^/items$", std::bind(&coco_server::create_item, this, network::placeholders::request));
        add_route(network::Delete, "^/items/.*$", std::bind(&coco_server::delete_item, this, network::placeholders::request));

        add_route(network::Get, "^/data/.*$", std::bind(&coco_server::get_data, this, network::placeholders::request));
        add_route(network::Post, "^/data/.*$", std::bind(&coco_server::set_datum, this, network::placeholders::request));

        add_route(network::Get, "^/fake/.*$", std::bind(&coco_server::fake, this, network::placeholders::request));

        add_route(network::Get, "^/reactive_rules$", std::bind(&coco_server::get_reactive_rules, this, network::placeholders::request));
        add_route(network::Post, "^/reactive_rules$", std::bind(&coco_server::create_reactive_rule, this, network::placeholders::request));

        add_route(network::Get, "^/openapi\\.json$", std::bind(&coco_server::get_openapi_spec, this, network::placeholders::request));

        json::json schemas{{"property",
                            {{"oneOf", std::vector<json::json>{{"$ref", "#/components/schemas/int_property"}, {"$ref", "#/components/schemas/float_property"}, {"$ref", "#/components/schemas/string_property"}, {"$ref", "#/components/schemas/symbol_property"}, {"$ref", "#/components/schemas/item_property"}, {"$ref", "#/components/schemas/json_property"}}}}},
                           {"int_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"int"}}}},
                               {"default", {{"type", "integer"}}},
                               {"min", {{"type", "integer"}}},
                               {"max", {{"type", "integer"}}}}},
                             {"required", std::vector<json::json>{"type"}}}},
                           {"float_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"float"}}}},
                               {"default", {{"type", "number"}}},
                               {"min", {{"type", "number"}}},
                               {"max", {{"type", "number"}}}}},
                             {"required", std::vector<json::json>{"type"}}}},
                           {"string_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"string"}}}},
                               {"default", {{"type", "string"}}}}},
                             {"required", std::vector<json::json>{"type"}}}},
                           {"symbol_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"symbol"}}}},
                               {"values", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                               {"multiple", {{"type", "boolean"}}},
                               {"default", {{"oneOf", std::vector<json::json>{{{"type", "string"}}, {{"type", "array"}, {"items", {{"type", "string"}}}}}}}}}},
                             {"required", std::vector<json::json>{"type"}}}},
                           {"item_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"item"}}}},
                               {"domain", {{"type", "string"}, {"format", "uuid"}}},
                               {"values", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                               {"multiple", {{"type", "boolean"}}},
                               {"default", {{"oneOf", std::vector<json::json>{{{"type", "string"}, {"format", "uuid"}}, {{"type", "array"}, {"items", {{"type", "string"}, {"format", "uuid"}}}}}}}}}},
                             {"required", std::vector<json::json>{"type", "type_id"}}}},
                           {"json_property",
                            {{"type", "object"},
                             {"properties",
                              {{"type", {{"type", "string"}, {"enum", {"json"}}}},
                               {"schema", {{"type", "object"}}},
                               {"default", {{"type", "object"}}}}},
                             {"required", std::vector<json::json>{"type", "schema"}}}},
                           {"type",
                            {{"type", "object"},
                             {"description", "A " COCO_NAME " type is a collection of static and dynamic properties that define the structure and behavior of items."},
                             {"properties",
                              {{"name", {{"type", "string"}}},
                               {"parents", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                               {"static_properties", {{"type", "array"}, {"items", {"$ref", "#/components/schemas/property"}}}},
                               {"dynamic_properties", {{"type", "array"}, {"items", {"$ref", "#/components/schemas/property"}}}},
                               {"data", {{"type", "object"}}}}},
                             {"required", {"name"}}}},
                           {"item",
                            {{"type", "object"},
                             {"description", "An item is an instance of a type, which can have static properties and dynamic data."},
                             {"properties",
                              {{"id", {{"type", "string"}, {"format", "uuid"}}},
                               {"type", {{"type", "string"}}},
                               {"properties", {{"type", "object"}, {"description", "Static properties of the item defined by its type."}}}}}}},
                           {"data",
                            {{"type", "object"},
                             {"properties",
                              {{"data", {{"type", "object"}}}}}}}};

        openapi_spec = json::json({{"openapi", "3.1.0"},
                                   {"info",
                                    {{"title", COCO_NAME " Server API"},
                                     {"version", "1.0.0"},
                                     {"description", "API for the " COCO_NAME " server."}}},
                                   {"components",
                                    {{"schemas", schemas}}},
                                   {"paths", {}}});
        LOG_DEBUG(openapi_spec.dump());
    }

    void coco_server::broadcast(json::json &&msg)
    {
        for (auto &[_, mod] : modules)
            mod->broadcast(msg);
    }

    void coco_server::created_type(const type &tp)
    {
        auto j_tp = tp.to_json();
        j_tp["msg_type"] = "new_type";
        j_tp["name"] = tp.get_name();
        broadcast(std::move(j_tp));
    }
    void coco_server::created_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "new_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void coco_server::updated_item(const item &itm)
    {
        auto j_itm = itm.to_json();
        j_itm["msg_type"] = "updated_item";
        j_itm["id"] = itm.get_id();
        broadcast(std::move(j_itm));
    }
    void coco_server::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) { broadcast({{"msg_type", "new_data"}, {"id", itm.get_id().c_str()}, {"value", {{"data", data}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()}}}}); }

    std::unique_ptr<network::response> coco_server::index(const network::request &) { return std::make_unique<network::file_response>(CLIENT_DIR "/dist/index.html"); }
    std::unique_ptr<network::response> coco_server::assets(const network::request &req)
    {
        std::string target = req.get_target();
        if (target.find('?') != std::string::npos)
            target = target.substr(0, target.find('?'));
        return std::make_unique<network::file_response>(CLIENT_DIR "/dist" + target);
    }

    std::unique_ptr<network::response> coco_server::get_types(const network::request &)
    {
        json::json ts(json::json_type::array);
        for (auto &tp : get_coco().get_types())
        {
            auto j_tp = tp.get().to_json();
            j_tp["name"] = tp.get().get_name();
            ts.push_back(std::move(j_tp));
        }
        return std::make_unique<network::json_response>(std::move(ts));
    }
    std::unique_ptr<network::response> coco_server::get_type(const network::request &req)
    {
        try
        { // get type by name in the path
            auto &tp = get_coco().get_type(req.get_target().substr(7));
            auto j_tp = tp.to_json();
            j_tp["name"] = tp.get_name();
            return std::make_unique<network::json_response>(std::move(j_tp));
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::create_type(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        std::string name = body["name"];

        std::vector<std::reference_wrapper<const type>> parents;
        if (body.contains("parents"))
        {
            if (body["parents"].get_type() != json::json_type::array)
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            for (auto &p : body["parents"].as_array())
            {
                if (p.get_type() != json::json_type::string)
                    return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
                try
                {
                    auto &tp = get_coco().get_type(static_cast<std::string>(p));
                    parents.emplace_back(tp);
                }
                catch (const std::exception &)
                {
                    return std::make_unique<network::json_response>(json::json({{"message", "Parent type not found"}}), network::status_code::not_found);
                }
            }
        }

        json::json static_props;
        if (body.contains("static_properties"))
            static_props = std::move(body["static_properties"]);

        json::json dynamic_props;
        if (body.contains("dynamic_properties"))
            dynamic_props = std::move(body["dynamic_properties"]);

        json::json data;
        if (body.contains("data"))
            data = std::move(body["data"]);

        [[maybe_unused]] auto &tp = get_coco().create_type(name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
        return std::make_unique<network::response>(network::status_code::no_content);
    }
    std::unique_ptr<network::response> coco_server::delete_type(const network::request &req)
    {
        try
        { // get type by name in the path
            get_coco().delete_type(get_coco().get_type(req.get_target().substr(7)));
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_items(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &itm : get_coco().get_items())
        {
            auto j_itm = itm.get().to_json();
            j_itm["id"] = itm.get().get_id();
            is.push_back(std::move(j_itm));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }
    std::unique_ptr<network::response> coco_server::get_item(const network::request &req)
    {
        try
        { // get item by id in the path
            auto &itm = get_coco().get_item(req.get_target().substr(7));
            auto j_tp = itm.to_json();
            j_tp["id"] = itm.get_id();
            return std::make_unique<network::json_response>(std::move(j_tp));
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }
    std::unique_ptr<network::response> coco_server::create_item(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("type") || body["type"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);

        std::string type_name = body["type"];
        type *tp;
        try
        {
            tp = &get_coco().get_type(type_name);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }
        try
        {
            json::json props = body.contains("properties") ? body["properties"] : json::json();
            auto &itm = get_coco().create_item(*tp, std::move(props));
            return std::make_unique<network::string_response>(std::string(itm.get_id()), network::status_code::created);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
    std::unique_ptr<network::response> coco_server::delete_item(const network::request &req)
    {
        try
        { // get item by id in the path
            get_coco().delete_item(get_coco().get_item(req.get_target().substr(7)));
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
    }

    std::unique_ptr<network::response> coco_server::get_data(const network::request &req)
    { // get item by id in the path
        auto id = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (id.find('?') != std::string::npos)
        {
            params = network::parse_query(id.substr(id.find('?') + 1));
            id = id.substr(0, id.find('?'));
        }
        item *itm;
        try
        {
            itm = &get_coco().get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        std::chrono::system_clock::time_point to = params.count("to") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("to"))}) : std::chrono::system_clock::time_point();
        std::chrono::system_clock::time_point from = params.count("from") ? std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("from"))}) : to - std::chrono::hours{24 * 7};
        return std::make_unique<network::json_response>(get_coco().get_values(*itm, from, to));
    }
    std::unique_ptr<network::response> coco_server::set_datum(const network::request &req)
    { // get item by id in the path
        auto id = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (id.find('?') != std::string::npos)
        {
            params = network::parse_query(id.substr(id.find('?') + 1));
            id = id.substr(0, id.find('?'));
        }
        item *itm;
        try
        {
            itm = &get_coco().get_item(id);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Item not found"}}), network::status_code::not_found);
        }
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        if (params.count("timestamp"))
        {
            std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::time_point(std::chrono::milliseconds{std::stol(params.at("timestamp"))});
            get_coco().set_value(*itm, json::json(body), timestamp);
        }
        else
            get_coco().set_value(*itm, json::json(body));
        return std::make_unique<network::response>(network::status_code::no_content);
    }

    std::unique_ptr<network::response> coco_server::fake(const network::request &req)
    {
        auto name = req.get_target().substr(6);
        std::map<std::string, std::string> params;
        if (name.find('?') != std::string::npos)
        {
            params = network::parse_query(name.substr(name.find('?') + 1));
            name = name.substr(0, name.find('?'));
        }
        type *tp;
        try
        {
            tp = &get_coco().get_type(name);
        }
        catch (const std::exception &)
        {
            return std::make_unique<network::json_response>(json::json({{"message", "Type not found"}}), network::status_code::not_found);
        }

        std::unordered_map<std::string, std::reference_wrapper<property>> props;
        std::queue<const type *> q;
        q.push(tp);
        while (!q.empty())
        {
            auto tp = q.front();
            q.pop();
            for (const auto &[name, p] : tp->get_dynamic_properties())
                props.emplace(name, *p);
            for (const auto &[_, p] : tp->get_parents())
                q.push(&p.get());
        }

        json::json j;

        if (params.count("parameters"))
            try
            {
                const auto pars = json::load(network::decode(params.at("parameters")));
                for (const auto &par : pars.as_array())
                    if (auto it = props.find(static_cast<std::string>(par)); it != props.end())
                        j[it->first] = it->second.get().fake();
                    else
                        return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            }
            catch (const std::exception &)
            {
                return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
            }
        else // fake all properties
            for (auto &[name, prop] : props)
                j[name] = prop.get().fake();

        return std::make_unique<network::json_response>(std::move(j));
    }

    std::unique_ptr<network::response> coco_server::get_reactive_rules(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &rr : get_coco().get_reactive_rules())
        {
            auto j_rrs = rr.get().to_json();
            j_rrs["name"] = rr.get().get_name();
            is.push_back(std::move(j_rrs));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }
    std::unique_ptr<network::response> coco_server::create_reactive_rule(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("content") || body["content"].get_type() != json::json_type::string)
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string content = body["content"];
        try
        {
            get_coco().create_reactive_rule(name, content);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }

    std::unique_ptr<network::response> coco_server::get_openapi_spec(const network::request &) { return std::make_unique<network::json_response>(openapi_spec); }
} // namespace coco
