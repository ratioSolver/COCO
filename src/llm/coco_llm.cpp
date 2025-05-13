#include "coco_llm.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    llm::llm(coco &cc, std::string_view host, unsigned short port) noexcept : listener(cc), client(host, port)
    {
        LOG_TRACE(intent_deftemplate);
        Build(get_env(), intent_deftemplate);

        LOG_DEBUG(entity_deftemplate);
        Build(get_env(), entity_deftemplate);

        AddUDF(get_env(), "understand", "v", 2, 2, "ys", understand, "understand", this);

        auto res = client.get("/version");
        if (!res || res->get_status_code() != network::ok)
            LOG_ERR("Failed to connect to the LLM server");
        else
            LOG_DEBUG("Connected to the LLM server " << static_cast<network::json_response &>(*res).get_body());
    }

    void understand(Environment *env, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Understanding..");

        auto &l = *reinterpret_cast<llm *>(udfc->context);

        UDFValue item_id;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        UDFValue message;
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        [[maybe_unused]] auto &itm = l.cc.get_item(item_id.lexemeValue->contents);

        std::string prompt = "You are a natural language understanding model. Given a user input, perform the following tasks:\n1. Intent Recognition: Identify which of the following intents (one or more) are present in the user's input.\nPossible intents and their descriptions are:\n";
        for (const auto &[name, intent] : l.intents)
            prompt += "\n- " + name + ": " + intent.get_description();
        prompt += "\n2. Extract the following entities from the user's input, if present. For each entity, provide the name and value.\nPossible entities, their types and descriptions are:\n";
        for (const auto &[name, entity] : l.entities)
            switch (entity.get_type())
            {
            case string_type:
                prompt += "\n- " + name + " (string): " + entity.get_description();
                break;
            case symbol_type:
                prompt += "\n- " + name + " (symbol): " + entity.get_description();
                break;
            case integer_type:
                prompt += "\n- " + name + " (integer): " + entity.get_description();
                break;
            case float_type:
                prompt += "\n- " + name + " (float): " + entity.get_description();
                break;
            case boolean_type:
                prompt += "\n- " + name + " (boolean): " + entity.get_description();
                break;
            default:
                LOG_ERR("Unknown entity type: " << entity.get_type());
                return;
            }
        prompt += "\n3. Provide a JSON response with the following structure:\n";
        prompt += "{\n\"intents\": [\n {\"name\": \"intent_name\"},\n],\n\"entities\": [\n{\"entity\": \"entity_name\", \"value\": \"entity_value\"},\n]\n}\n";
        prompt += "Instructions:\n- Use only the given intent and entity definitions.\n- Ignore any information not covered by the possible intents or entities.\n- If a value is not found for an entity, omit that entity.\n- Entities should match the specified type exactly.\n";

        json::json context(json::json_type::array);
        context.push_back({{"role", "system"}, {"content", prompt.c_str()}});
        context.push_back({{"role", "user"}, {"content", message.lexemeValue->contents}});

        auto res = l.client.post("/llm", {{"context", std::move(context)}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        for (const auto &intent : json_res.get_body()["intents"].as_array())
        {
            auto &intent_name = static_cast<const std::string>(intent["name"]);

            FactBuilder *intent_fact_builder = CreateFactBuilder(env, "intent");
            FBPutSlotSymbol(intent_fact_builder, "item_id", item_id.lexemeValue->contents);
            FBPutSlotSymbol(intent_fact_builder, "name", intent_name.c_str());

            auto intent_fact = FBAssert(intent_fact_builder);
            assert(intent_fact);
            LOG_TRACE(l.to_string(intent_fact));
            FBDispose(intent_fact_builder);
        }

        for (const auto &entity : json_res.get_body()["entities"].as_array())
        {
            auto &entity_name = static_cast<const std::string>(entity["entity"]);

            FactBuilder *entity_fact_builder = CreateFactBuilder(env, "entity");
            FBPutSlotSymbol(entity_fact_builder, "item_id", item_id.lexemeValue->contents);
            FBPutSlotSymbol(entity_fact_builder, "name", entity_name.c_str());
            switch (l.entities.at(entity_name).get_type())
            {
            case string_type:
                FBPutSlotString(entity_fact_builder, "value", static_cast<std::string>(entity["value"]).c_str());
                break;
            case symbol_type:
                FBPutSlotSymbol(entity_fact_builder, "value", static_cast<std::string>(entity["value"]).c_str());
                break;
            case integer_type:
                FBPutSlotInteger(entity_fact_builder, "value", static_cast<int64_t>(entity["value"]));
                break;
            case float_type:
                FBPutSlotFloat(entity_fact_builder, "value", static_cast<double>(entity["value"]));
                break;
            case boolean_type:
                FBPutSlotSymbol(entity_fact_builder, "value", static_cast<bool>(entity["value"]) ? "TRUE" : "FALSE");
                break;
            default:
                LOG_ERR("Unknown entity type: " << l.entities.at(entity_name).get_type());
                return;
            }

            auto entity_fact = FBAssert(entity_fact_builder);
            assert(entity_fact);
            LOG_TRACE(llm.to_string(entity_fact));
            FBDispose(entity_fact_builder);
        }
    }
} // namespace coco
