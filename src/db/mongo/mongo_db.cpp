#include "mongo_db.hpp"
#include "logging.hpp"
#include "crypto.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_module::mongo_module(mongo_db &db) noexcept : db_module(db) {}
    mongocxx::database &mongo_module::get_db() const noexcept { return static_cast<mongo_db &>(db).db; }

    mongo_db::mongo_db(json::json &&cnfg, std::string_view mongodb_uri) noexcept : coco_db(std::move(cnfg)), conn(mongocxx::uri(mongodb_uri.data())), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), items_collection(db["items"]), item_data_collection(db["item_data"]), reactive_rules_collection(db["reactive_rules"])
    {
        assert(conn);
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

        assert(db);
        assert(types_collection);
        assert(items_collection);
        assert(item_data_collection);
        assert(reactive_rules_collection);

        if (item_data_collection.list_indexes().begin() == item_data_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for item data collection");
            item_data_collection.create_index(bsoncxx::builder::stream::document{} << "item_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (reactive_rules_collection.list_indexes().begin() == reactive_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for reactive rules collection");
            reactive_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    void mongo_db::drop() noexcept
    {
        coco_db::drop();
        db.drop();
    }

    std::vector<db_type> mongo_db::get_types() noexcept
    {
        std::vector<db_type> types;
        for (const auto &doc : types_collection.find({}))
        {
            auto name = doc["_id"].get_string().value;
            std::vector<std::string> parents;
            if (doc.find("parents") != doc.end())
                for (const auto &p : doc["parents"].get_array().value)
                    parents.push_back(p.get_string().value.data());

            std::optional<json::json> data, static_props, dynamic_props;
            if (doc.find("data") != doc.end())
                data = json::load(bsoncxx::to_json(doc["data"].get_document().view()));
            if (doc.find("static_properties") != doc.end())
                static_props = json::load(bsoncxx::to_json(doc["static_properties"].get_document().view()));
            if (doc.find("dynamic_properties") != doc.end())
                dynamic_props = json::load(bsoncxx::to_json(doc["dynamic_properties"].get_document().view()));

            types.push_back({std::string(name), std::move(parents), std::move(data), std::move(static_props), std::move(dynamic_props)});
        }
        return types;
    }
    void mongo_db::create_type(std::string_view name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("_id", name.data()));
        if (!parents.empty())
        {
            bsoncxx::builder::basic::array parents_builder;
            for (const auto &parent : parents)
                parents_builder.append(parent);
            doc.append(bsoncxx::builder::basic::kvp("parents", parents_builder));
        }
        if (!data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        if (!static_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::from_json(static_props.dump())));
        if (!dynamic_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::from_json(dynamic_props.dump())));
        if (!types_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert type: " + std::string(name));
    }
    void mongo_db::set_parents(std::string_view name, const std::vector<std::string> &parents)
    {
        bsoncxx::builder::basic::document doc;
        if (parents.empty())
            doc.append(bsoncxx::builder::basic::kvp("$unset", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("parents", ""))));
        else
        {
            bsoncxx::builder::basic::array arr;
            for (const auto &p : parents)
                arr.append(p);
            doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("parents", arr))));
        }
        if (!types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", name.data())), doc.view()))
            throw std::invalid_argument("Failed to update type parents");
    }
    void mongo_db::delete_type(std::string_view name)
    {
        bsoncxx::builder::basic::array ids_builder;
        for (const auto &item : items_collection.find(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("type", name.data())), mongocxx::options::find{}.projection(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", 1)))))
            ids_builder.append(item["_id"].get_value());
        if (!item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$in", ids_builder))))))
            throw std::invalid_argument("Failed to delete item data of items of type: " + std::string(name));
        if (!items_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("type", name.data()))))
            throw std::invalid_argument("Failed to delete items of type: " + std::string(name));
        if (!types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", name.data()))))
            throw std::invalid_argument("Failed to delete type: " + std::string(name));
        for (const auto &other_type : types_collection.find(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("parents", name.data())))) // Remove the type from the "parents" array of these types
            types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", other_type["_id"].get_value())), bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("parents", name.data())))));
    }

    [[nodiscard]] std::vector<db_item> mongo_db::get_items() noexcept
    {
        std::vector<db_item> items;
        for (const auto &doc : items_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto type = doc["type"].get_string().value;

            std::optional<json::json> props;
            if (doc.find("properties") != doc.end())
                props = json::load(bsoncxx::to_json(doc["properties"].get_document().view()));

            std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value;
            if (doc.find("value") != doc.end())
                value = {json::load(bsoncxx::to_json(doc["value"]["data"].get_document().view())), doc["value"]["timestamp"].get_date()};

            items.push_back({std::move(id), std::string(type), std::move(props), std::move(value)});
        }
        return items;
    }
    std::string mongo_db::create_item(std::string_view type, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type", type.data()));
        if (!props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        if (val.has_value())
        {
            bsoncxx::builder::basic::document data_doc;
            data_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(val->first.dump())));
            data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{val->second}));
            doc.append(bsoncxx::builder::basic::kvp("value", data_doc));
        }
        auto result = items_collection.insert_one(doc.view());
        if (!result)
            throw std::invalid_argument("Failed to insert " + std::string(type) + " item");
        return result->inserted_id().get_oid().value.to_string();
    }
    void mongo_db::set_properties(std::string_view itm_id, const json::json &props)
    {
        bsoncxx::builder::basic::document filter_doc;
        filter_doc.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{itm_id.data()}));

        bsoncxx::builder::basic::document update_fields;
        update_fields.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));

        bsoncxx::builder::basic::document update_doc;
        update_doc.append(bsoncxx::builder::basic::kvp("$set", update_fields.view()));

        if (!items_collection.update_one(filter_doc.view(), update_doc.view()))
            throw std::invalid_argument("Failed to set properties for item: " + std::string(itm_id));
    }
    json::json mongo_db::get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        bsoncxx::builder::basic::document query;
        query.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}));
        query.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$gte", bsoncxx::types::b_date{from}), bsoncxx::builder::basic::kvp("$lte", bsoncxx::types::b_date{to}))));
        json::json data = json::json_type::array;
        for (const auto &doc : item_data_collection.find(query.view()))
            data.push_back(json::json{{"data", json::load(bsoncxx::to_json(doc["data"].get_document().view()))}, {"timestamp", doc["timestamp"].get_date().to_int64()}});
        return data;
    }
    void mongo_db::set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        bsoncxx::builder::basic::document filter_doc;
        filter_doc.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}));
        filter_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));

        bsoncxx::builder::basic::document update_fields;
        update_fields.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(val.dump())));

        bsoncxx::builder::basic::document update_doc;
        update_doc.append(bsoncxx::builder::basic::kvp("$set", update_fields.view()));

        mongocxx::options::update update_opts;
        update_opts.upsert(true); // Create a new document if no document matches the filter

        auto result = item_data_collection.update_one(filter_doc.view(), update_doc.view(), update_opts);
        if (!result)
            throw std::invalid_argument("Failed to merge or insert data for item: " + std::string(itm_id));
    }
    void mongo_db::delete_item(std::string_view itm_id)
    {
        if (!item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}))))
            throw std::invalid_argument("Failed to delete item data for item: " + std::string(itm_id));
        if (!items_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{itm_id.data()}))))
            throw std::invalid_argument("Failed to delete item: " + std::string(itm_id));
    }

    std::vector<db_rule> mongo_db::get_reactive_rules() noexcept
    {
        std::vector<db_rule> rules;
        for (const auto &doc : reactive_rules_collection.find({}))
        {
            auto name = doc["name"].get_string().value;
            auto content = doc["content"].get_string().value;
            rules.push_back({std::string(name), std::string(content)});
        }
        return rules;
    }
    void mongo_db::create_reactive_rule(std::string_view rule_name, std::string_view rule_content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", rule_name.data()));
        doc.append(bsoncxx::builder::basic::kvp("content", rule_content.data()));
        if (!reactive_rules_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert reactive rule: " + std::string(rule_name));
    }
} // namespace coco
