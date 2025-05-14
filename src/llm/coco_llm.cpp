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

        std::string prompt = "You are a natural language understanding model. Given a user input, perform the following tasks:\n";
        prompt += "\n1. Intent Recognition\n";
        prompt += "Identify the intents present in the user's input. For each intent, provide the name.\n";
        prompt += "The intents are defined as follows:\n";
        for (const auto &[name, intent] : intents)
            prompt += " - " + name + ": " + intent->get_description() + "\n";
        prompt += "\n2. Entity Extraction\n";
        prompt += "Extract the entities from the user's input. For each entity, provide the name and value.\n";
        prompt += "The entities are defined as follows:\n";
        for (const auto &[name, entity] : entities)
            prompt += " - " + name + " (" + entity_type_to_string(entity->get_type()) + "): " + entity->get_description() + "\n";
        prompt += "\n3. Response Format\n";
        prompt += "The response should contain the recognized intents and extracted entities from the user's input. The values of the entities should be in the same format as specified in the possible entities.\n";
        prompt += "The response should be a valid JSON object. The keys in the JSON object should be 'intents' and 'entities'.\n";
        prompt += "The 'intents' key should contain an array of recognized intents. The 'entities' key should contain an array array of objects, each with a 'entity' and 'value' key.\n";
        prompt += "Do NOT include any other text or explanation in the response.\n";
        LOG_TRACE("Prompt:\n"
                  << prompt);

        json::json context(json::json_type::array);
        context.push_back({{"role", "system"}, {"content", prompt.c_str()}});
        context.push_back({{"role", "user"}, {"content", message}});
        auto res = client.post("/llm", {{"context", std::move(context)}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            return;
        }
        auto llm_res = static_cast<std::string>(static_cast<network::json_response &>(*res).get_body()["content"]);
        LOG_TRACE("Response:\n"
                  << llm_res);
        json::json j_res = json::load(llm_res);
        for (const auto &intent : j_res["intents"].as_array())
        {
            auto &intent_name = static_cast<const std::string>(intent);
            if (intents.find(intent_name) == intents.end())
                LOG_WARN("Intent " << intent_name << " not found");

            FactBuilder *intent_fact_builder = CreateFactBuilder(get_env(), "intent");
            FBPutSlotSymbol(intent_fact_builder, "item_id", item.get_id().c_str());
            FBPutSlotSymbol(intent_fact_builder, "name", intent_name.c_str());

            auto intent_fact = FBAssert(intent_fact_builder);
            assert(intent_fact);
            LOG_TRACE(to_string(intent_fact));
            FBDispose(intent_fact_builder);
        }
        for (const auto &entity : j_res["entities"].as_array())
        {
            auto &entity_name = static_cast<const std::string>(entity["entity"]);
            if (entities.find(entity_name) == entities.end())
                LOG_WARN("Entity " << entity_name << " not found");

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
