#include "llm_db.hpp"
#include "logging.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    llm_db::llm_db(mongo_db &db) noexcept : mongo_module(db), intents_collection(get_db()["intents"]), entities_collection(get_db()["entities"]), slots_collection(get_db()["slots"])
    {
        assert(intents_collection);
        assert(entities_collection);
        assert(slots_collection);

        if (intents_collection.list_indexes().begin() == intents_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for intents collection");
            intents_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (entities_collection.list_indexes().begin() == entities_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for entities collection");
            entities_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (slots_collection.list_indexes().begin() == slots_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for slots collection");
            slots_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    std::vector<db_intent> llm_db::get_intents() noexcept
    {
        std::vector<db_intent> intents;
        for (const auto &doc : intents_collection.find({}))
        {
            auto name = doc["name"].get_string().value;
            auto description = doc["description"].get_string().value;
            intents.push_back({std::string(name), std::string(description)});
        }
        return intents;
    }
    void llm_db::create_intent(std::string_view name, std::string_view description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name.data()));
        doc.append(bsoncxx::builder::basic::kvp("description", description.data()));
        if (!intents_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert intent: " + std::string(name));
    }

    std::vector<db_entity> llm_db::get_entities() noexcept
    {
        std::vector<db_entity> entities;
        for (const auto &doc : entities_collection.find({}))
        {
            auto type = doc["type"].get_int32().value;
            auto name = doc["name"].get_string().value;
            auto description = doc["description"].get_string().value;
            entities.push_back({type, std::string(name), std::string(description)});
        }
        return entities;
    }
    void llm_db::create_entity(int type, std::string_view name, std::string_view description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type", type));
        doc.append(bsoncxx::builder::basic::kvp("name", name.data()));
        doc.append(bsoncxx::builder::basic::kvp("description", description.data()));
        if (!entities_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert entity: " + std::string(name));
    }

    std::vector<db_slot> llm_db::get_slots() noexcept
    {
        std::vector<db_slot> slots;
        for (const auto &doc : slots_collection.find({}))
        {
            auto type = doc["type"].get_int32().value;
            auto name = doc["name"].get_string().value;
            auto description = doc["description"].get_string().value;
            auto influence_context = doc["influence_context"].get_bool().value;
            slots.push_back({type, std::string(name), std::string(description), influence_context});
        }
        return slots;
    }
    void llm_db::create_slot(int type, std::string_view name, std::string_view description, bool influence_context)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type", type));
        doc.append(bsoncxx::builder::basic::kvp("name", name.data()));
        doc.append(bsoncxx::builder::basic::kvp("description", description.data()));
        doc.append(bsoncxx::builder::basic::kvp("influence_context", influence_context));
        if (!slots_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert slot: " + std::string(name));
    }
} // namespace coco
