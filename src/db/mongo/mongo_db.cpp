#include "mongo_db.hpp"
#include "logging.hpp"
#include "crypto.hpp"
#include <mongocxx/client.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_module::mongo_module(mongo_db &db) noexcept : db_module(db) {}
    [[nodiscard]] mongocxx::v_noabi::pool::entry mongo_module::get_client() const noexcept { return static_cast<mongo_db &>(db).pool.acquire(); }

    mongo_db::mongo_db(json::json &&cnfg, std::string_view mongodb_uri) noexcept : coco_db(std::move(cnfg)), pool(mongocxx::uri(mongodb_uri.data())), db_name(config["name"].get<std::string>())
    {
        LOG_DEBUG("Connecting to MongoDB at " + std::string(mongodb_uri));
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        assert(db);

        auto item_data_collection = db[item_data_collection_name];
        assert(item_data_collection);
        if (item_data_collection.list_indexes().begin() == item_data_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for item data collection");
            item_data_collection.create_index(bsoncxx::builder::stream::document{} << "item_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        auto rules_collection = db[rules_collection_name];
        assert(rules_collection);
        if (rules_collection.list_indexes().begin() == rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for rules collection");
            rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    std::vector<db_type> mongo_db::get_types() noexcept
    {
        std::vector<db_type> types;
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto types_collection = db[types_collection_name];
        assert(types_collection);
        // Sort by _id to have a deterministic order (item properties require domain types to be already defined)..
        for (const auto &doc : types_collection.find({}, mongocxx::options::find{}.sort(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", 1)))))
        {
            auto name = doc["_id"].get_string().value;
            std::optional<json::json> static_props, dynamic_props, data;
            if (doc.find("static_properties") != doc.end())
                static_props = json::load(bsoncxx::to_json(doc["static_properties"].get_document().view()));
            if (doc.find("dynamic_properties") != doc.end())
                dynamic_props = json::load(bsoncxx::to_json(doc["dynamic_properties"].get_document().view()));
            if (doc.find("data") != doc.end())
                data = json::load(bsoncxx::to_json(doc["data"].get_document().view()));

            types.push_back({std::string(name), std::move(static_props), std::move(dynamic_props), std::move(data)});
        }
        return types;
    }
    void mongo_db::create_type(std::string_view name, const json::json &static_props, const json::json &dynamic_props, const json::json &data)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("_id", name.data()));
        if (!static_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::from_json(static_props.dump())));
        if (!dynamic_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::from_json(dynamic_props.dump())));
        if (!data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto types_collection = db[types_collection_name];
        assert(types_collection);
        if (!types_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert type: " + std::string(name));
    }
    void mongo_db::set_properties(std::string_view tp_name, const json::json &static_props, const json::json &dynamic_props)
    {
        bsoncxx::builder::basic::document update_fields; // Fields to set
        if (!static_props.as_object().empty())
            update_fields.append(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::from_json(static_props.dump())));
        if (!dynamic_props.as_object().empty())
            update_fields.append(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::from_json(dynamic_props.dump())));

        bsoncxx::builder::basic::document filter_doc; // Prepare the filter document
        filter_doc.append(bsoncxx::builder::basic::kvp("_id", tp_name.data()));

        bsoncxx::builder::basic::document update_doc; // Prepare the update document
        update_doc.append(bsoncxx::builder::basic::kvp("$set", update_fields.view()));

        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto types_collection = db[types_collection_name];
        assert(types_collection);
        if (!types_collection.update_one(filter_doc.view(), update_doc.view()))
            throw std::invalid_argument("Failed to update type: " + std::string(tp_name));
    }
    void mongo_db::delete_type(std::string_view name)
    {
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        if (!items_collection.update_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("types", name.data())), bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("types", name.data()))))))
            throw std::invalid_argument("Failed to remove type from items: " + std::string(name));
        auto types_collection = db[types_collection_name];
        assert(types_collection);
        if (!types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", name.data()))))
            throw std::invalid_argument("Failed to delete type: " + std::string(name));
    }

    [[nodiscard]] std::vector<db_item> mongo_db::get_items() noexcept
    {
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        std::vector<db_item> items;
        for (const auto &doc : items_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            std::vector<std::string> types;
            if (doc.find("types") != doc.end() && doc["types"].type() == bsoncxx::type::k_array)
                for (const auto &type_elem : doc["types"].get_array().value)
                    types.push_back(type_elem.get_string().value.data());

            std::optional<json::json> props;
            if (doc.find("properties") != doc.end())
                props = json::load(bsoncxx::to_json(doc["properties"].get_document().view()));

            std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value;
            if (doc.find("value") != doc.end())
                value = {json::load(bsoncxx::to_json(doc["value"]["data"].get_document().view())), doc["value"]["timestamp"].get_date()};

            items.push_back({std::move(id), std::move(types), std::move(props), std::move(value)});
        }
        return items;
    }
    std::string mongo_db::create_item(const std::vector<std::string> &types, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val)
    {
        bsoncxx::builder::basic::document doc;
        bsoncxx::builder::basic::array types_array;
        for (const auto &type : types)
            types_array.append(type);
        doc.append(bsoncxx::builder::basic::kvp("types", types_array));
        if (!props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        if (val.has_value())
        {
            bsoncxx::builder::basic::document data_doc;
            data_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(val->first.dump())));
            data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{val->second}));
            doc.append(bsoncxx::builder::basic::kvp("value", data_doc));
        }
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        auto result = items_collection.insert_one(doc.view());
        if (!result)
            throw std::invalid_argument("Failed to insert item");
        return result->inserted_id().get_oid().value.to_string();
    }
    void mongo_db::set_properties(std::string_view itm_id, const json::json &props)
    {
        bsoncxx::builder::basic::document update_fields; // Fields to set
        // Iterate through properties and build set/unset operations
        for (const auto &[nm, prop] : props.as_object())
            switch (prop.get_type())
            {
            case json::json_type::null:
                update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, bsoncxx::types::b_null{}));
                break;
            case json::json_type::boolean:
                update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, prop.get<bool>()));
                break;
            case json::json_type::number:
                if (prop.is_float())
                    update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, prop.get<double>()));
                else
                    update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, prop.get<int64_t>()));
                break;
            case json::json_type::string:
                update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, prop.get<std::string>()));
                break;
            case json::json_type::array:
                update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, to_bson_array(prop).view()));
                break;
            default:
                update_fields.append(bsoncxx::builder::basic::kvp("properties." + nm, bsoncxx::from_json(prop.dump())));
            }

        bsoncxx::builder::basic::document filter_doc; // Prepare the filter document
        filter_doc.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{itm_id.data()}));
        bsoncxx::builder::basic::document update_doc; // Prepare the update document
        update_doc.append(bsoncxx::builder::basic::kvp("$set", update_fields.view()));

        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        if (!items_collection.update_one(filter_doc.view(), update_doc.view()))
            throw std::invalid_argument("Failed to set properties for item: " + std::string(itm_id));
    }
    json::json mongo_db::get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        bsoncxx::builder::basic::document query;
        query.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}));
        query.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$gte", bsoncxx::types::b_date{from}), bsoncxx::builder::basic::kvp("$lte", bsoncxx::types::b_date{to}))));
        json::json data = json::json_type::array;

        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto item_data_collection = db[item_data_collection_name];
        assert(item_data_collection);
        mongocxx::options::find find_opts;
        find_opts.sort(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("timestamp", 1)));
        for (const auto &doc : item_data_collection.find(query.view(), find_opts))
            data.push_back(json::json{{"data", json::load(bsoncxx::to_json(doc["data"].get_document().view()))}, {"timestamp", doc["timestamp"].get_date().to_int64()}});
        return data;
    }
    void mongo_db::set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        bsoncxx::builder::basic::document update_fields;
        bsoncxx::builder::basic::document update_val_fields; // Fields to set
        // Iterate through properties and build set/unset operations
        for (const auto &[nm, v] : val.as_object())
            switch (v.get_type())
            {
            case json::json_type::null:
                update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, bsoncxx::types::b_null{}));
                update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, bsoncxx::types::b_null{}));
                break;
            case json::json_type::boolean:
                update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, v.get<bool>()));
                update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, v.get<bool>()));
                break;
            case json::json_type::number:
                if (v.is_float())
                {
                    update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, v.get<double>()));
                    update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, v.get<double>()));
                }
                else
                {
                    update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, v.get<int64_t>()));
                    update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, v.get<int64_t>()));
                }
                break;
            case json::json_type::string:
                update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, v.get<std::string>()));
                update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, v.get<std::string>()));
                break;
            case json::json_type::array:
                update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, to_bson_array(v).view()));
                update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, to_bson_array(v).view()));
                break;
            default:
                update_fields.append(bsoncxx::builder::basic::kvp("value.data." + nm, bsoncxx::from_json(v.dump())));
                update_val_fields.append(bsoncxx::builder::basic::kvp("data." + nm, bsoncxx::from_json(v.dump())));
            }
        update_fields.append(bsoncxx::builder::basic::kvp("value.timestamp", bsoncxx::types::b_date{timestamp}));
        bsoncxx::builder::basic::document filter_doc; // Prepare the filter document
        filter_doc.append(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{itm_id.data()}));
        bsoncxx::builder::basic::document update_doc;
        update_doc.append(bsoncxx::builder::basic::kvp("$set", update_fields.view()));

        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        if (!items_collection.update_one(filter_doc.view(), update_doc.view()))
            throw std::invalid_argument("Failed to set value for item: " + std::string(itm_id));

        bsoncxx::builder::basic::document filter_data_doc; // Prepare the filter document
        filter_data_doc.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}));
        filter_data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        bsoncxx::builder::basic::document update_data_doc; // Prepare the update document
        update_data_doc.append(bsoncxx::builder::basic::kvp("$set", update_val_fields.view()));
        mongocxx::options::update update_opts;
        update_opts.upsert(true); // Create a new document if no document matches the filter

        auto item_data_collection = db[item_data_collection_name];
        assert(item_data_collection);
        if (!item_data_collection.update_one(filter_data_doc.view(), update_data_doc.view(), update_opts))
            throw std::invalid_argument("Failed to set value for item: " + std::string(itm_id));
    }
    void mongo_db::delete_item(std::string_view itm_id)
    {
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto item_data_collection = db[item_data_collection_name];
        assert(item_data_collection);
        if (!item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{itm_id.data()}))))
            throw std::invalid_argument("Failed to delete item data for item: " + std::string(itm_id));
        auto items_collection = db[items_collection_name];
        assert(items_collection);
        if (!items_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{itm_id.data()}))))
            throw std::invalid_argument("Failed to delete item: " + std::string(itm_id));
    }

    std::vector<db_rule> mongo_db::get_rules() noexcept
    {
        std::vector<db_rule> rules;
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto rules_collection = db[rules_collection_name];
        assert(rules_collection);
        for (const auto &doc : rules_collection.find({}))
        {
            auto name = doc["name"].get_string().value;
            auto content = doc["content"].get_string().value;
            rules.push_back({std::string(name), std::string(content)});
        }
        return rules;
    }
    void mongo_db::create_rule(std::string_view rule_name, std::string_view rule_content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", rule_name.data()));
        doc.append(bsoncxx::builder::basic::kvp("content", rule_content.data()));
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        auto rules_collection = db[rules_collection_name];
        assert(rules_collection);
        if (!rules_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert rule: " + std::string(rule_name));
    }

    void mongo_db::drop() noexcept
    {
        coco_db::drop();
        auto client = pool.acquire();
        auto db = (*client)[db_name];
        db.drop();
    }

    bsoncxx::array::value to_bson_array(const json::json &j)
    {
        bsoncxx::builder::basic::array arr;
        for (const auto &elem : j.as_array())
            switch (elem.get_type())
            {
            case json::json_type::null:
                arr.append(bsoncxx::types::b_null{});
                break;
            case json::json_type::boolean:
                arr.append(elem.get<bool>());
                break;
            case json::json_type::number:
                if (elem.is_float())
                    arr.append(elem.get<double>());
                else
                    arr.append(elem.get<int64_t>());
                break;
            case json::json_type::string:
                arr.append(elem.get<std::string>());
                break;
            case json::json_type::object:
                arr.append(bsoncxx::from_json(elem.dump()));
                break;
            case json::json_type::array:
                arr.append(to_bson_array(elem).view());
                break;
            }
        return arr.extract();
    }
} // namespace coco
