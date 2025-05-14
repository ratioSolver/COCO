#include "coco_llm.hpp"
#include "coco.hpp"
#include "llm_db.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    coco_llm::coco_llm(coco &cc, std::string_view host, unsigned short port) noexcept : coco_module(cc), client(host, port)
    {
        LOG_TRACE(intent_deftemplate);
        Build(get_env(), intent_deftemplate);

        LOG_DEBUG(entity_deftemplate);
        Build(get_env(), entity_deftemplate);

        AddUDF(get_env(), "understand", "v", 2, 2, "ys", understand_udf, "understand", this);

        auto res = client.get("/version");
        if (!res || res->get_status_code() != network::ok)
            LOG_ERR("Failed to connect to the LLM server");
        else
            LOG_DEBUG("Connected to the LLM server " << static_cast<network::json_response &>(*res).get_body());

        auto &db = cc.get_db().add_module<llm_db>(static_cast<mongo_db &>(cc.get_db()));
        auto ints = db.get_intents();
        LOG_DEBUG("Retrieved " << ints.size() << " intents");
        for (const auto &c_intent : ints)
            intents.emplace(c_intent.name, utils::make_u_ptr<intent>(c_intent.name, c_intent.description));
        auto ents = db.get_entities();
        LOG_DEBUG("Retrieved " << ents.size() << " entities");
        for (const auto &c_entity : ents)
            entities.emplace(c_entity.name, utils::make_u_ptr<entity>(static_cast<entity_type>(c_entity.type), c_entity.name, c_entity.description));
    }

    std::vector<utils::ref_wrapper<intent>> coco_llm::get_intents() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<utils::ref_wrapper<intent>> result;
        for (const auto &[name, intent] : intents)
            result.emplace_back(*intent);
        return result;
    }
    void coco_llm::create_intent(std::string_view name, std::string_view description)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        cc.get_db().get_module<llm_db>().create_intent(name, description);
        if (!intents.emplace(name, utils::make_u_ptr<intent>(name, description)).second)
            throw std::runtime_error("Intent already exists");
    }

    std::vector<utils::ref_wrapper<entity>> coco_llm::get_entities() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<utils::ref_wrapper<entity>> result;
        for (const auto &[name, entity] : entities)
            result.emplace_back(*entity);
        return result;
    }
    void coco_llm::create_entity(std::string_view name, std::string_view description, entity_type type)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        cc.get_db().get_module<llm_db>().create_entity(name, description, type);
        if (!entities.emplace(name, utils::make_u_ptr<entity>(type, name, description)).second)
            throw std::runtime_error("Entity already exists");
    }

    void coco_llm::understand(item &item, std::string_view message) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());

        std::string prompt = "You are a natural language understanding model. Given a user input, perform the following tasks:\n1. Intent Recognition: Identify which of the following intents (one or more) are present in the user's input.\nPossible intents and their descriptions are:\n";
        for (const auto &[name, intent] : intents)
            prompt += "\n- " + name + ": " + intent->get_description();
        prompt += "\n2. Extract the following entities from the user's input, if present. For each entity, provide the name and value.\nPossible entities, their types and descriptions are:\n";
        for (const auto &[name, entity] : entities)
            prompt += "\n- " + name + " (" + to_string(entity->get_type()) + "): " + entity->get_description();
        prompt += "\n3. Provide a JSON response with the following structure:\n";
        prompt += "{\n\"intents\": [\n {\"name\": \"intent_name\"},\n],\n\"entities\": [\n{\"entity\": \"entity_name\", \"value\": \"entity_value\"},\n]\n}\n";
        prompt += "Instructions:\n- Use only the given intent and entity definitions.\n- Ignore any information not covered by the possible intents or entities.\n- If a value is not found for an entity, omit that entity.\n- Entities should match the specified type exactly.\n";

        json::json context(json::json_type::array);
        context.push_back({{"role", "system"}, {"content", prompt.c_str()}});
        context.push_back({{"role", "user"}, {"content", message}});
        auto res = client.post("/llm", {{"context", std::move(context)}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            return;
        }
        auto &json_res = static_cast<network::json_response &>(*res);
        for (const auto &intent : json_res.get_body()["intents"].as_array())
        {
            auto &intent_name = static_cast<const std::string>(intent["name"]);

            FactBuilder *intent_fact_builder = CreateFactBuilder(get_env(), "intent");
            FBPutSlotSymbol(intent_fact_builder, "item_id", item.get_id().c_str());
            FBPutSlotSymbol(intent_fact_builder, "name", intent_name.c_str());

            auto intent_fact = FBAssert(intent_fact_builder);
            assert(intent_fact);
            LOG_TRACE(to_string(intent_fact));
            FBDispose(intent_fact_builder);
        }
        for (const auto &entity : json_res.get_body()["entities"].as_array())
        {
            auto &entity_name = static_cast<const std::string>(entity["entity"]);

            FactBuilder *entity_fact_builder = CreateFactBuilder(get_env(), "entity");
            FBPutSlotSymbol(entity_fact_builder, "item_id", item.get_id().c_str());
            FBPutSlotSymbol(entity_fact_builder, "name", entity_name.c_str());
            switch (entities.at(entity_name)->get_type())
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
            }

            auto entity_fact = FBAssert(entity_fact_builder);
            assert(entity_fact);
            LOG_TRACE(to_string(entity_fact));
            FBDispose(entity_fact_builder);
        }
    }

    intent::intent(std::string_view name, std::string_view description) : name(name), description(description) {}
    entity::entity(entity_type type, std::string_view name, std::string_view description) : type(type), name(name), description(description) {}

    void understand_udf(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Understanding..");

        auto &llm = *reinterpret_cast<coco_llm *>(udfc->context);

        UDFValue item_id;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        UDFValue message;
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        llm.understand(llm.cc.get_item(item_id.lexemeValue->contents), message.lexemeValue->contents);
    }
} // namespace coco
