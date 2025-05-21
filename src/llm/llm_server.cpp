#include "llm_server.hpp"

namespace coco
{
    llm_server::llm_server(coco_server &srv, coco_llm &llm) noexcept : server_module(srv), llm_listener(llm), llm(llm)
    {
        srv.add_route(network::Get, "^/intents$", std::bind(&llm_server::get_intents, this, network::placeholders::request));
        srv.add_route(network::Get, "^/entities$", std::bind(&llm_server::get_entities, this, network::placeholders::request));
    }

    void llm_server::intent_created([[maybe_unused]] const intent &i)
    {
        auto j_it = i.to_json();
        j_it["msg_type"] = "new_intent";
        srv.broadcast(std::move(j_it));
    }
    void llm_server::entity_created([[maybe_unused]] const entity &e)
    {
        auto j_it = e.to_json();
        j_it["msg_type"] = "new_entity";
        srv.broadcast(std::move(j_it));
    }
    void llm_server::slot_created([[maybe_unused]] const slot &s)
    {
        auto j_it = s.to_json();
        j_it["msg_type"] = "new_slot";
        srv.broadcast(std::move(j_it));
    }

    utils::u_ptr<network::response> llm_server::get_intents(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &it : llm.get_intents())
        {
            auto j_it = it->to_json();
            j_it["name"] = it->get_name();
            is.push_back(std::move(j_it));
        }
        return utils::make_u_ptr<network::json_response>(std::move(is));
    }

    utils::u_ptr<network::response> llm_server::create_intent(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("description") || body["description"].get_type() != json::json_type::string)
            return utils::make_u_ptr<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string description = body["description"];
        try
        {
            llm.create_intent(name, description);
            return utils::make_u_ptr<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return utils::make_u_ptr<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }

    utils::u_ptr<network::response> llm_server::get_entities(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &it : llm.get_entities())
        {
            auto j_it = it->to_json();
            j_it["name"] = it->get_name();
            is.push_back(std::move(j_it));
        }
        return utils::make_u_ptr<network::json_response>(std::move(is));
    }

    utils::u_ptr<network::response> llm_server::create_entity(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (body.get_type() != json::json_type::object || !body.contains("type") || body["type"].get_type() != json::json_type::string || !body.contains("name") || body["name"].get_type() != json::json_type::string || !body.contains("description") || body["description"].get_type() != json::json_type::string)
            return utils::make_u_ptr<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string type = body["type"];
        data_type type_name;
        if (type == "string")
            type_name = data_type::string_type;
        else if (type == "symbol")
            type_name = data_type::symbol_type;
        else if (type == "int")
            type_name = data_type::integer_type;
        else if (type == "float")
            type_name = data_type::float_type;
        else if (type == "bool")
            type_name = data_type::boolean_type;
        else
            return utils::make_u_ptr<network::json_response>(json::json({{"message", "Invalid type"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string description = body["description"];
        try
        {
            llm.create_entity(type_name, name, description);
            return utils::make_u_ptr<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return utils::make_u_ptr<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
} // namespace coco
