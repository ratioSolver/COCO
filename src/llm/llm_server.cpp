#include "llm_server.hpp"

namespace coco
{
    llm_server::llm_server(coco_server &srv, coco_llm &llm) noexcept : server_module(srv), llm_listener(llm), llm(llm)
    {
        srv.add_route(network::Get, "^/intents$", std::bind(&llm_server::get_intents, this, network::placeholders::request));
        srv.add_route(network::Post, "^/intents$", std::bind(&llm_server::create_intent, this, network::placeholders::request));
        srv.add_route(network::Get, "^/entities$", std::bind(&llm_server::get_entities, this, network::placeholders::request));
        srv.add_route(network::Post, "^/entities$", std::bind(&llm_server::create_entity, this, network::placeholders::request));

        // Define schemas for intents and entities
        get_schemas()["intent"] = {
            {"type", "object"},
            {"description", "An intent with a name and description."},
            {"properties",
             {{"name", {{"type", "string"}, {"description", "The name of the intent."}}},
              {"description", {{"type", "string"}, {"description", "The description of the intent."}}}}},
            {"required", std::vector<json::json>{"name", "description"}}};
        get_schemas()["entity"] = {
            {"type", "object"},
            {"description", "An entity with a type, name, and description."},
            {"properties",
             {{"type", {{"type", "string"}, {"description", "The type of the entity (e.g., string, symbol, int, float, bool)."}}},
              {"name", {{"type", "string"}, {"description", "The name of the entity."}}},
              {"description", {{"type", "string"}, {"description", "The description of the entity."}}}}},
            {"required", std::vector<json::json>{"type", "name", "description"}}};

        // Define OpenAPI paths for intents and entities
        get_paths()["/intents"] = {
            {"get",
             {{"summary", "Get all intents."},
              {"description", "Endpoint to retrieve all intents."},
              {"responses",
               {{"200", {{"description", "List of intents."}, {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/intent"}}}}}}}}}}},
                {"401", {{"description", "Unauthorized."}}}}}}},
            {"post",
             {{"summary", "Create a new intent."},
              {"description", "Endpoint to create a new intent."},
              {"requestBody",
               {{"required", true},
                {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/intent"}}}}}}}}},
              {"responses",
               {{"201", {{"description", "Intent created successfully."}}},
                {"400", {{"description", "Invalid request."}}},
                {"401", {{"description", "Unauthorized."}}},
                {"409", {{"description", "Intent already exists."}}}}}}}};
        get_paths()["/entities"] = {
            {"get",
             {{"summary", "Get all entities."},
              {"description", "Endpoint to retrieve all entities."},
              {"responses",
               {{"200", {{"description", "List of entities."}, {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/entity"}}}}}}}}}}},
                {"401", {{"description", "Unauthorized."}}}}}}},
            {"post",
             {{"summary", "Create a new entity."},
              {"description", "Endpoint to create a new entity."},
              {"requestBody",
               {{"required", true},
                {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/entity"}}}}}}}}},
              {"responses",
               {{"201", {{"description", "Entity created successfully."}}},
                {"400", {{"description", "Invalid request."}}},
                {"401", {{"description", "Unauthorized."}}},
                {"409", {{"description", "Entity already exists."}}}}}}}};
    }

    void llm_server::created_intent(const intent &i)
    {
        auto j_it = i.to_json();
        j_it["msg_type"] = "new_intent";
        srv.broadcast(std::move(j_it));
    }
    void llm_server::created_entity(const entity &e)
    {
        auto j_it = e.to_json();
        j_it["msg_type"] = "new_entity";
        srv.broadcast(std::move(j_it));
    }
    void llm_server::created_slot(const slot &s)
    {
        auto j_it = s.to_json();
        j_it["msg_type"] = "new_slot";
        srv.broadcast(std::move(j_it));
    }

    std::unique_ptr<network::response> llm_server::get_intents(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &it : llm.get_intents())
        {
            auto j_it = it.get().to_json();
            j_it["name"] = it.get().get_name();
            is.push_back(std::move(j_it));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }

    std::unique_ptr<network::response> llm_server::create_intent(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("name") || !body["name"].is_string() || !body.contains("description") || !body["description"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string description = body["description"];
        try
        {
            llm.create_intent(name, description);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }

    std::unique_ptr<network::response> llm_server::get_entities(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &it : llm.get_entities())
        {
            auto j_it = it.get().to_json();
            j_it["name"] = it.get().get_name();
            is.push_back(std::move(j_it));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }

    std::unique_ptr<network::response> llm_server::create_entity(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("type") || !body["type"].is_string() || !body.contains("name") || !body["name"].is_string() || !body.contains("description") || !body["description"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
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
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid type"}}), network::status_code::bad_request);
        std::string name = body["name"];
        std::string description = body["description"];
        try
        {
            llm.create_entity(type_name, name, description);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
} // namespace coco
