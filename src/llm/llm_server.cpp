#include "llm_server.hpp"

namespace coco
{
    llm_server::llm_server(coco_server &srv, coco_llm &llm) noexcept : server_module(srv), llm_listener(llm), llm(llm)
    {
        srv.add_route(network::Get, "^/intents$", std::bind(&llm_server::get_intents, this, network::placeholders::request));
        srv.add_route(network::Post, "^/intents$", std::bind(&llm_server::create_intent, this, network::placeholders::request));
        srv.add_route(network::Get, "^/entities$", std::bind(&llm_server::get_entities, this, network::placeholders::request));
        srv.add_route(network::Post, "^/entities$", std::bind(&llm_server::create_entity, this, network::placeholders::request));

        // Define schemas for intents, entities and slots
        add_schema("intent", {{"type", "object"},
                              {"description", "An intent with a name and description."},
                              {"properties",
                               {{"name", {{"type", "string"}, {"description", "The name of the intent."}}},
                                {"description", {{"type", "string"}, {"description", "The description of the intent."}}}}},
                              {"required", std::vector<json::json>{"name", "description"}}});
        add_schema("entity", {{"type", "object"},
                              {"description", "An entity with a type, name, and description."},
                              {"properties",
                               {{"type", {{"type", "string"}, {"description", "The type of the entity."}, {"enum", {"string", "symbol", "int", "float", "bool"}}}},
                                {"name", {{"type", "string"}, {"description", "The name of the entity."}}},
                                {"description", {{"type", "string"}, {"description", "The description of the entity."}}}}},
                              {"required", std::vector<json::json>{"type", "name", "description"}}});
        add_schema("slot", {{"type", "object"},
                            {"description", "A slot with a type, name, and description."},
                            {"properties",
                             {{"type", {{"type", "string"}, {"description", "The type of the slot."}, {"enum", {"string", "symbol", "int", "float", "bool"}}}},
                              {"name", {{"type", "string"}, {"description", "The name of the slot."}}},
                              {"description", {{"type", "string"}, {"description", "The description of the slot."}}}}},
                            {"required", std::vector<json::json>{"type", "name", "description"}}});

        // Define OpenAPI paths for intents, entities and slots
        add_path("/intents", {{"get",
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
                                  {"409", {{"description", "Intent already exists."}}}}}}}});
        add_path("/entities", {{"get",
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
                                   {"409", {{"description", "Entity already exists."}}}}}}}});
        add_path("/slots", {{"get",
                             {{"summary", "Get all slots."},
                              {"description", "Endpoint to retrieve all slots."},
                              {"responses",
                               {{"200", {{"description", "List of slots."}, {"content", {{"application/json", {{"schema", {{"type", "array"}, {"items", {{"$ref", "#/components/schemas/slot"}}}}}}}}}}},
                                {"401", {{"description", "Unauthorized."}}}}}}},
                            {"post",
                             {{"summary", "Create a new slot."},
                              {"description", "Endpoint to create a new slot."},
                              {"requestBody",
                               {{"required", true},
                                {"content", {{"application/json", {{"schema", {{"$ref", "#/components/schemas/slot"}}}}}}}}},
                              {"responses",
                               {{"201", {{"description", "Slot created successfully."}}},
                                {"400", {{"description", "Invalid request."}}},
                                {"401", {{"description", "Unauthorized."}}},
                                {"409", {{"description", "Slot already exists."}}}}}}}});

#ifdef BUILD_AUTH
        get_path("/intents")["get"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        get_path("/intents")["post"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        get_path("/entities")["get"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        get_path("/entities")["post"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        get_path("/slots")["get"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        get_path("/slots")["post"]["security"] = std::vector<json::json>{{"bearerAuth", std::vector<json::json>{}}};
        add_authorized_path("/intents", network::verb::Get, {0, 1});
        add_authorized_path("/intents", network::verb::Post, {0});
        add_authorized_path("/entities", network::verb::Get, {0, 1});
        add_authorized_path("/entities", network::verb::Post, {0});
        add_authorized_path("/slots", network::verb::Get, {0, 1});
        add_authorized_path("/slots", network::verb::Post, {0});
#endif
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
        data_type type_name = string_to_type(body["type"].get<std::string>());
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

    std::unique_ptr<network::response> llm_server::get_slots(const network::request &)
    {
        json::json is(json::json_type::array);
        for (auto &it : llm.get_slots())
        {
            auto j_it = it.get().to_json();
            j_it["name"] = it.get().get_name();
            is.push_back(std::move(j_it));
        }
        return std::make_unique<network::json_response>(std::move(is));
    }

    std::unique_ptr<network::response> llm_server::create_slot(const network::request &req)
    {
        auto &body = static_cast<const network::json_request &>(req).get_body();
        if (!body.is_object() || !body.contains("type") || !body["type"].is_string() || !body.contains("name") || !body["name"].is_string() || !body.contains("description") || !body["description"].is_string())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        if (body.contains("influence_context") && !body["influence_context"].is_boolean())
            return std::make_unique<network::json_response>(json::json({{"message", "Invalid request"}}), network::status_code::bad_request);
        data_type type_name = string_to_type(body["type"].get<std::string>());
        std::string name = body["name"];
        std::string description = body["description"];
        bool influence_context = body.contains("influence_context") ? body["influence_context"].get<bool>() : true;
        try
        {
            llm.create_slot(type_name, name, description, influence_context);
            return std::make_unique<network::response>(network::status_code::no_content);
        }
        catch (const std::exception &e)
        {
            return std::make_unique<network::json_response>(json::json({{"message", e.what()}}), network::status_code::conflict);
        }
    }
} // namespace coco
