#include "mongo_db.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(json::json &&cnfg, const std::string &mongodb_uri) noexcept : coco_db(std::move(cnfg)), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), items_collection(db["items"])
    {
        assert(conn);
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));
    }

    void mongo_db::drop() noexcept
    {
        LOG_WARN("Dropping database..");
        db.drop();
    }

    std::vector<db_type> mongo_db::get_types() noexcept
    {
        std::vector<db_type> types;
        for (const auto &doc : types_collection.find({}))
        {
            auto name = doc["_id"].get_string().value.to_string();
            std::vector<std::string> parents;
            if (doc.find("parents") != doc.end())
                for (const auto &p : doc["parents"].get_array().value)
                    parents.push_back(p.get_oid().value.to_string());

            std::optional<json::json> data, static_props, dynamic_props;
            if (doc.find("data") != doc.end())
                data = json::load(bsoncxx::to_json(doc["data"].get_document().view()));
            if (doc.find("static_properties") != doc.end())
                static_props = json::load(bsoncxx::to_json(doc["static_properties"].get_document().view()));
            if (doc.find("dynamic_properties") != doc.end())
                dynamic_props = json::load(bsoncxx::to_json(doc["dynamic_properties"].get_document().view()));

            types.push_back({std::move(name), std::move(parents), std::move(data), std::move(static_props), std::move(dynamic_props)});
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
        if (!types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", name.data()))))
            throw std::invalid_argument("Failed to delete type: " + std::string(name));
    }

    [[nodiscard]] std::vector<db_item> mongo_db::get_items() noexcept
    {
        std::vector<db_item> items;
        for (const auto &doc : types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto type = doc["type"].get_string().value.to_string();

            std::optional<json::json> props;
            if (doc.find("properties") != doc.end())
                props = json::load(bsoncxx::to_json(doc["data"].get_document().view()));

            std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> value;
            if (doc.find("value") != doc.end())
                value = {json::load(bsoncxx::to_json(doc["value"]["data"].get_document().view())), doc["value"]["timestamp"].get_date()};

            items.push_back({std::move(id), std::move(type), std::move(props), std::move(value)});
        }
        return items;
    }
    std::string mongo_db::create_item(std::string_view type, const json::json &props, const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type", type.data()));
        if (!props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        if (!val.as_object().empty())
        {
            bsoncxx::builder::basic::document data_doc;
            data_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(val.dump())));
            data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
            doc.append(bsoncxx::builder::basic::kvp("value", data_doc));
        }
        auto result = items_collection.insert_one(doc.view());
        if (!result)
            throw std::invalid_argument("Failed to insert " + std::string(type) + " item");
        return result->inserted_id().get_oid().value.to_string();
    }
} // namespace coco
