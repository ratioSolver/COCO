#include "coco_llm.hpp"
#include "coco.hpp"
#include "llm_db.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef BUILD_LISTENERS
#define CREATED_INTENT(i) created_intent(i)
#define CREATED_ENTITY(e) created_entity(e)
#define CREATED_SLOT(s) created_slot(s)
#else
#define CREATED_INTENT(i)
#define CREATED_ENTITY(e)
#define CREATED_SLOT(s)
#endif

namespace coco
{
    coco_llm::coco_llm(coco &cc, std::string_view host, unsigned short port) noexcept : coco_module(cc), client(host, port)
    {
        LOG_TRACE(intent_deftemplate);
        [[maybe_unused]] auto build_intent_dt_err = Build(get_env(), intent_deftemplate);
        assert(build_intent_dt_err == BE_NO_ERROR);
        LOG_TRACE(entity_deftemplate);
        [[maybe_unused]] auto build_entity_dt_err = Build(get_env(), entity_deftemplate);
        assert(build_entity_dt_err == BE_NO_ERROR);
        LOG_TRACE(slot_deftemplate);
        [[maybe_unused]] auto build_slot_dt_err = Build(get_env(), slot_deftemplate);
        assert(build_slot_dt_err == BE_NO_ERROR);

        [[maybe_unused]] auto understand_err = AddUDF(get_env(), "understand", "v", 2, 2, "ys", understand_udf, "understand_udf", this);
        assert(understand_err == AUE_NO_ERROR);
        [[maybe_unused]] auto add_data_err = AddUDF(get_env(), "set_slots", "v", 3, 4, "ymml", set_slots_udf, "set_slots_udf", this);
        assert(add_data_err == AUE_NO_ERROR);

        auto &db = get_coco().get_db().add_module<llm_db>(static_cast<mongo_db &>(get_coco().get_db()));
        auto ints = db.get_intents();
        LOG_DEBUG("Retrieved " << ints.size() << " intents");
        for (const auto &c_intent : ints)
            intents.emplace(c_intent.name, std::make_unique<intent>(c_intent.name, c_intent.description));
        auto ents = db.get_entities();
        LOG_DEBUG("Retrieved " << ents.size() << " entities");
        for (const auto &c_entity : ents)
            entities.emplace(c_entity.name, std::make_unique<entity>(static_cast<data_type>(c_entity.type), c_entity.name, c_entity.description));
        auto slts = db.get_slots();
        LOG_DEBUG("Retrieved " << slts.size() << " slots");
        for (const auto &c_slot : slts)
            slots.emplace(c_slot.name, std::make_unique<slot>(static_cast<data_type>(c_slot.type), c_slot.name, c_slot.description, c_slot.influence_context));
    }

    std::vector<std::reference_wrapper<intent>> coco_llm::get_intents() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<std::reference_wrapper<intent>> result;
        for (const auto &[name, intent] : intents)
            result.emplace_back(*intent);
        return result;
    }
    void coco_llm::create_intent(std::string_view name, std::string_view description, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        get_coco().get_db().get_module<llm_db>().create_intent(name, description);
        auto it = intents.emplace(name, std::make_unique<intent>(name, description));
        if (!it.second)
            throw std::runtime_error("Intent already exists");
        else
            CREATED_INTENT(*it.first->second);
        if (infere)
            Run(get_env(), -1);
    }

    std::vector<std::reference_wrapper<entity>> coco_llm::get_entities() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<std::reference_wrapper<entity>> result;
        for (const auto &[name, entity] : entities)
            result.emplace_back(*entity);
        return result;
    }
    void coco_llm::create_entity(data_type type, std::string_view name, std::string_view description, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        get_coco().get_db().get_module<llm_db>().create_entity(type, name, description);
        auto it = entities.emplace(name, std::make_unique<entity>(type, name, description));
        if (!it.second)
            throw std::runtime_error("Entity already exists");
        else
            CREATED_ENTITY(*it.first->second);
        if (infere)
            Run(get_env(), -1);
    }

    std::vector<std::reference_wrapper<slot>> coco_llm::get_slots() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<std::reference_wrapper<slot>> result;
        for (const auto &[name, slot] : slots)
            result.emplace_back(*slot);
        return result;
    }
    void coco_llm::create_slot(data_type type, std::string_view name, std::string_view description, bool influence_context, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        get_coco().get_db().get_module<llm_db>().create_slot(type, name, description, influence_context);
        auto it = slots.emplace(name, std::make_unique<slot>(type, name, description, influence_context));
        if (!it.second)
            throw std::runtime_error("Slot already exists");
        else
            CREATED_SLOT(*it.first->second);
        if (infere)
            Run(get_env(), -1);
    }

    void coco_llm::set_slots(item &item, json::json &&new_slots, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());

        for (const auto &[slot_name, val] : new_slots.as_object())
            if (val.is_null())
            {
                assert(slot_facts.count(item.get_id()));
                assert(slot_facts.at(item.get_id()).count(slot_name));
                assert(current_slots.count(item.get_id()));
                assert(current_slots.at(item.get_id()).contains(slot_name));
                Retract(slot_facts.at(item.get_id()).at(slot_name));
                slot_facts.at(item.get_id()).erase(slot_name);
                current_slots.at(item.get_id()).erase(slot_name);
            }
            else
            {
                FactBuilder *slot_fact_builder = CreateFactBuilder(get_env(), "slot");
                FBPutSlotSymbol(slot_fact_builder, "item_id", item.get_id().c_str());
                FBPutSlotSymbol(slot_fact_builder, "name", slot_name.c_str());
                switch (this->slots.at(slot_name)->get_type())
                {
                case string_type:
                    FBPutSlotString(slot_fact_builder, "value", static_cast<const std::string>(val).c_str());
                    break;
                case symbol_type:
                    FBPutSlotSymbol(slot_fact_builder, "value", static_cast<const std::string>(val).c_str());
                    break;
                case integer_type:
                    FBPutSlotInteger(slot_fact_builder, "value", static_cast<int64_t>(val));
                    break;
                case float_type:
                    FBPutSlotFloat(slot_fact_builder, "value", static_cast<double>(val));
                    break;
                case boolean_type:
                    FBPutSlotSymbol(slot_fact_builder, "value", static_cast<bool>(val) ? "TRUE" : "FALSE");
                    break;
                default:
                    LOG_WARN("Unknown type for slot " + slot_name);
                    break;
                }
                auto slot_fact = FBAssert(slot_fact_builder);
                assert(slot_fact);
                LOG_TRACE(to_string(slot_fact));
                FBDispose(slot_fact_builder);
                slot_facts[item.get_id()][slot_name] = slot_fact;
                current_slots[item.get_id()][slot_name] = val;
            }
        if (infere)
            Run(get_env(), -1);
    }

    void coco_llm::understand(item &item, std::string_view message, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::string prompt = "#Task\nYou are an AI model which given the current state and a user input, perform the following tasks:\n";
        prompt += "1. Identify the intents present in the user's input.\n";
        prompt += "2. Extract the entities mentioned in the user's input.\n";
        prompt += "3. Return the results in a structured JSON format.\n";
        prompt += "## Current State\n";
        if (auto it = current_slots.find(item.get_id()); it != current_slots.end())
        {
            prompt += "The current state of the conversation is:\n";
            for (const auto &[slot_name, val] : it->second.as_object())
                prompt += "- " + slot_name + "(" + slots.at(slot_name)->get_description() + "): " + static_cast<std::string>(val) + "\n";
        }
        else
            prompt += "The current state of the conversation is empty.\n";
        prompt += "## Intent Recognition\n";
        prompt += "Identify all intents present in the user's input. For each intent, return only its name.\n";
        prompt += "Available intents:\n";
        for (const auto &[name, intent] : intents)
            prompt += "- " + name + ": " + intent->get_description() + "\n";
        prompt += "## Entity Extraction\n";
        prompt += "Extract all entities mentioned in the user's input. For each entity, return its name and the extracted value.\n";
        prompt += "Available entities:\n";
        for (const auto &[name, entity] : entities)
            prompt += "- " + name + " (" + type_to_string(entity->get_type()) + "): " + entity->get_description() + "\n";
        prompt += "## Response Format\n";
        prompt += "Return the output as a JSON object with the following structure:\n";
        prompt += "{\n";
        prompt += "  \"intents\": [\"intent1\", \"intent2\", ...],\n";
        prompt += "  \"entities\": {\n";
        prompt += "    \"entity1\": \"value1\",\n";
        prompt += "    \"entity2\": \"value2\",\n";
        prompt += "    ...\n";
        prompt += "  }\n";
        prompt += "}\n";
        prompt += "The values of the extracted entities must follow the format specified in their definitions above.\n";
        prompt += "Do NOT include any additional formatting (such as '```') or explanations in the output. Return only the JSON object.\n";

        json::json j_prompt;
        j_prompt["model"] = LLM_MODEL;
        j_prompt["messages"] = std::vector<json::json>{{{"role", "system"}, {"content", prompt}}, {{"role", "user"}, {"content", message}}};
        j_prompt["stream"] = false;

        auto res = client.post("/" LLM_PROVIDER "/v3/openai/chat/completions", std::move(j_prompt), {{"Content-Type", "application/json"}, {"Authorization", "Bearer " LLM_API_KEY}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            LOG_ERR(*res);
            return;
        }
        auto llm_res = static_cast<network::json_response &>(*res).get_body();
        LOG_TRACE("Response:\n"
                  << llm_res);
        json::json j_res = json::load(static_cast<std::string>(llm_res["choices"][0]["message"]["content"]));
        for (const auto &intent : j_res["intents"].as_array())
        {
            auto intent_name = static_cast<const std::string>(intent);
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
        for (const auto &[name, value] : j_res["entities"].as_object())
        {
            if (entities.find(name) == entities.end())
                LOG_WARN("Entity " << name << " not found");

            FactBuilder *entity_fact_builder = CreateFactBuilder(get_env(), "entity");
            FBPutSlotSymbol(entity_fact_builder, "item_id", item.get_id().c_str());
            FBPutSlotSymbol(entity_fact_builder, "name", name.c_str());
            switch (entities.at(name)->get_type())
            {
            case string_type:
                FBPutSlotString(entity_fact_builder, "value", static_cast<const std::string>(value).c_str());
                break;
            case symbol_type:
                FBPutSlotSymbol(entity_fact_builder, "value", static_cast<const std::string>(value).c_str());
                break;
            case integer_type:
                FBPutSlotInteger(entity_fact_builder, "value", static_cast<int64_t>(value));
                break;
            case float_type:
                FBPutSlotFloat(entity_fact_builder, "value", static_cast<double>(value));
                break;
            case boolean_type:
                FBPutSlotSymbol(entity_fact_builder, "value", static_cast<bool>(value) ? "TRUE" : "FALSE");
                break;
            }

            auto entity_fact = FBAssert(entity_fact_builder);
            assert(entity_fact);
            LOG_TRACE(to_string(entity_fact));
            FBDispose(entity_fact_builder);
        }
        if (infere)
            Run(get_env(), -1);
    }

#ifdef BUILD_LISTENERS
    void coco_llm::created_intent(const intent &i)
    {
        for (auto &listener : listeners)
            listener->created_intent(i);
    }
    void coco_llm::created_entity(const entity &e)
    {
        for (auto &listener : listeners)
            listener->created_entity(e);
    }
    void coco_llm::created_slot(const slot &s)
    {
        for (auto &listener : listeners)
            listener->created_slot(s);
    }
#endif

    intent::intent(std::string_view name, std::string_view description) : name(name), description(description) {}
    json::json intent::to_json() const noexcept { return json::json{{"name", name}, {"description", description}}; }
    entity::entity(data_type type, std::string_view name, std::string_view description) : type(type), name(name), description(description) {}
    json::json entity::to_json() const noexcept { return json::json{{"type", type_to_string(type)}, {"name", name}, {"description", description}}; }
    slot::slot(data_type type, std::string_view name, std::string_view description, bool influence_context) : type(type), name(name), description(description), influence_context(influence_context) {}
    json::json slot::to_json() const noexcept { return json::json{{"type", type_to_string(type)}, {"name", name}, {"description", description}, {"influence_context", influence_context}}; }

    void set_slots_udf(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Setting slots..");

        auto &llm = *reinterpret_cast<coco_llm *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;

        UDFValue pars; // we get the parameters..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &pars))
            return;

        UDFValue vals; // we get the values..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &vals))
            return;

        json::json j_slots;
        for (size_t i = 0; i < pars.multifieldValue->length; ++i)
        {
            auto &par = pars.multifieldValue->contents[i];
            if (par.header->type != SYMBOL_TYPE)
                return;
            auto &val = vals.multifieldValue->contents[i];
            switch (val.header->type)
            {
            case INTEGER_TYPE:
                j_slots[par.lexemeValue->contents] = static_cast<int64_t>(val.integerValue->contents);
                break;
            case FLOAT_TYPE:
                j_slots[par.lexemeValue->contents] = val.floatValue->contents;
                break;
            case STRING_TYPE:
                j_slots[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            case SYMBOL_TYPE:
                if (std::string(val.lexemeValue->contents) == "TRUE")
                    j_slots[par.lexemeValue->contents] = true;
                else if (std::string(val.lexemeValue->contents) == "FALSE")
                    j_slots[par.lexemeValue->contents] = false;
                else if (std::string(val.lexemeValue->contents) == "nil")
                    j_slots[par.lexemeValue->contents] = nullptr;
                else
                    j_slots[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            default:
                return;
            }
        }

        llm.set_slots(llm.get_coco().get_item(item_id.lexemeValue->contents), std::move(j_slots), false);
    }

    void understand_udf(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Understanding..");

        auto &llm = *reinterpret_cast<coco_llm *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;

        UDFValue message; // we get the message..
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        llm.understand(llm.get_coco().get_item(item_id.lexemeValue->contents), message.lexemeValue->contents, false);
    }

#ifdef BUILD_LISTENERS
    llm_listener::llm_listener(coco_llm &llm) noexcept : llm(llm) { llm.listeners.emplace_back(this); }
    llm_listener::~llm_listener() { llm.listeners.erase(std::remove(llm.listeners.begin(), llm.listeners.end(), this), llm.listeners.end()); }
#endif
} // namespace coco
