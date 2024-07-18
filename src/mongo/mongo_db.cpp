#include "mongo_db.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), items_collection(db["items"]), item_data_collection(db["item_data"]), reactive_rules_collection(db["reactive_rules"]), deliberative_rules_collection(db["deliberative_rules"])
    {
        assert(conn);
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

        if (types_collection.list_indexes().begin() == types_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for types collection");
            types_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }
        if (items_collection.list_indexes().begin() == items_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for items collection");
            items_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }
        if (item_data_collection.list_indexes().begin() == item_data_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for item data collection");
            item_data_collection.create_index(bsoncxx::builder::stream::document{} << "item_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize);
        }
        if (reactive_rules_collection.list_indexes().begin() == reactive_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for reactive rules collection");
            reactive_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }
        if (deliberative_rules_collection.list_indexes().begin() == deliberative_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for deliberative rules collection");
            deliberative_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }
    }

    void mongo_db::init(coco_core &cc)
    {
        coco_db::init(cc);
        LOG_DEBUG("Retrieving all types from MongoDB");
        std::map<type *, std::vector<std::string>> parents;
        for (const auto &doc : types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            if (doc.find("parents") != doc.end())
                for (const auto &p : doc["parents"].get_array().value)
                    parents[&get_type(p.get_oid().value.to_string())].push_back(id);
            std::vector<std::unique_ptr<property>> static_properties;
            if (doc.find("static_properties") != doc.end())
                for (const auto &p : doc["static_properties"].get_array().value)
                {
                    auto prop = make_property(cc, json::load(bsoncxx::to_json(p.get_document().view())));
                    static_properties.emplace_back(std::move(prop));
                }
            std::vector<std::unique_ptr<property>> dynamic_properties;
            if (doc.find("dynamic_properties") != doc.end())
                for (const auto &p : doc["dynamic_properties"].get_array().value)
                {
                    auto prop = make_property(cc, json::load(bsoncxx::to_json(p.get_document().view())));
                    dynamic_properties.emplace_back(std::move(prop));
                }
            coco_db::create_type(cc, id, name, description, {}, std::move(static_properties), std::move(dynamic_properties));
        }
        for (const auto &[tp, p] : parents)
            for (const auto &id : p)
                add_parent(*tp, get_type(id));
        LOG_DEBUG("Retrieved " << get_types().size() << " types");

        LOG_DEBUG("Retrieving all items from MongoDB");
        for (const auto &doc : items_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto type_id = doc["type_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto properties = json::load(bsoncxx::to_json(doc["properties"].get_document().view()));
            coco_db::create_item(cc, id, get_type(type_id), name, properties);
        }
        LOG_DEBUG("Retrieved " << get_items().size() << " items");

        LOG_DEBUG("Retrieving reactive rules from MongoDB");
        for (const auto &doc : reactive_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_reactive_rule(cc, id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_reactive_rules().size() << " reactive rules");

        LOG_DEBUG("Retrieving deliberative rules from MongoDB");
        for (const auto &doc : deliberative_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_deliberative_rule(cc, id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_deliberative_rules().size() << " deliberative rules");
    }

    type &mongo_db::create_type(coco_core &cc, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        if (!parents.empty())
        {
            auto parents_array = bsoncxx::builder::basic::array{};
            for (const auto &p : parents)
                parents_array.append(bsoncxx::oid{p.get().get_id()});
            doc.append(bsoncxx::builder::basic::kvp("parents", parents_array));
        }
        if (!static_properties.empty())
        {
            auto static_props = bsoncxx::builder::basic::array{};
            for (const auto &p : static_properties)
                static_props.append(bsoncxx::from_json(to_json(*p).dump()));
            doc.append(bsoncxx::builder::basic::kvp("static_properties", static_props));
        }
        if (!dynamic_properties.empty())
        {
            auto dynamic_props = bsoncxx::builder::basic::array{};
            for (const auto &p : dynamic_properties)
                dynamic_props.append(bsoncxx::from_json(to_json(*p).dump()));
            doc.append(bsoncxx::builder::basic::kvp("dynamic_properties", dynamic_props));
        }
        auto result = types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_type(cc, result->inserted_id().get_oid().value.to_string(), name, description, std::move(parents), std::move(static_properties), std::move(dynamic_properties));
        throw std::invalid_argument("Failed to insert type: " + name);
    }

    void mongo_db::set_type_name(type &tp, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update type name: " + name);
        coco_db::set_type_name(tp, name);
    }
    void mongo_db::set_type_description(type &tp, const std::string &description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("description", description))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update type description: " + description);
        coco_db::set_type_description(tp, description);
    }
    void mongo_db::add_static_property(type &tp, std::unique_ptr<property> &&prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::from_json(to_json(*prop).dump())))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to add static property: " + prop->get_name());
        coco_db::add_static_property(tp, std::move(prop));
    }
    void mongo_db::remove_static_property(type &tp, const property &prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", prop.get_name()))))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to remove static property: " + prop.get_name());
        coco_db::remove_static_property(tp, prop);
    }
    void mongo_db::add_dynamic_property(type &tp, std::unique_ptr<property> &&prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::from_json(to_json(*prop).dump())))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to add dynamic property: " + prop->get_name());
        coco_db::add_dynamic_property(tp, std::move(prop));
    }
    void mongo_db::remove_dynamic_property(type &tp, const property &prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", prop.get_name()))))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to remove dynamic property: " + prop.get_name());
        coco_db::remove_dynamic_property(tp, prop);
    }

    void mongo_db::delete_type(const type &tp)
    {
        // Remove all items of this type
        for (const auto &it : get_items())
            if (&it.get().get_type() == &tp)
                delete_item(it);
        auto result = types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic ::kvp("_id", bsoncxx::oid{tp.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete type: " + tp.get_name());
        coco_db::delete_type(tp);
    }

    item &mongo_db::create_item(coco_core &cc, const type &tp, const std::string &name, const json::json &props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type_id", bsoncxx::oid{tp.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        auto result = items_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_item(cc, result->inserted_id().get_oid().value.to_string(), tp, name, props);
        throw std::invalid_argument("Failed to insert item: " + name);
    }

    void mongo_db::set_item_name(item &it, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        auto result = items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update item name: " + name);
        coco_db::set_item_name(it, name);
    }

    void mongo_db::set_item_properties(item &it, const json::json &props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())))));
        auto result = items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update item properties: " + props.dump());
        coco_db::set_item_properties(it, props);
    }

    void mongo_db::delete_item(const item &it)
    {
        auto result = item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete item data for item: " + it.get_name());
        result = items_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete item: " + it.get_name());
        coco_db::delete_item(it);
    }

    void mongo_db::add_data(const item &it, const std::chrono::system_clock::time_point &timestamp, const json::json &data)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        auto result = item_data_collection.insert_one(doc.view());
        if (!result)
            throw std::invalid_argument("Failed to insert data for item: " + it.get_name());
    }

    rule &mongo_db::create_reactive_rule(coco_core &cc, const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = reactive_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_reactive_rule(cc, result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert reactive rule: " + name);
    }

    rule &mongo_db::create_deliberative_rule(coco_core &cc, const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = deliberative_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_deliberative_rule(cc, result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert deliberative rule: " + name);
    }
} // namespace coco
