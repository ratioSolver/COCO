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

        new_parameter_converter<integer_parameter_converter>();
        new_parameter_converter<real_parameter_converter>();
        new_parameter_converter<boolean_parameter_converter>();
        new_parameter_converter<symbolic_parameter_converter>();
        new_parameter_converter<string_parameter_converter>();
        new_parameter_converter<array_parameter_converter>(*this);

        LOG_DEBUG("Retrieving types from MongoDB");
        for (const auto &doc : types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            std::unordered_map<std::string, std::unique_ptr<coco_parameter>> static_pars;
            for (const auto &p : doc["static_parameters"].get_array().value)
            {
                auto par = from_bson(p.get_document().view());
                static_pars[par->get_name()] = std::move(par);
            }
            std::unordered_map<std::string, std::unique_ptr<coco_parameter>> dynamic_pars;
            for (const auto &p : doc["dynamic_parameters"].get_array().value)
            {
                auto par = from_bson(p.get_document().view());
                dynamic_pars[par->get_name()] = std::move(par);
            }
            coco_db::create_type(id, name, description, std::move(static_pars), std::move(dynamic_pars));
        }
        LOG_DEBUG("Retrieved " << get_types().size() << " types");

        if (items_collection.list_indexes().begin() == items_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for items collection");
            items_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }

        LOG_DEBUG("Retrieving items from MongoDB");
        for (const auto &doc : items_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto type_id = doc["type_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto parameters = json::load(bsoncxx::to_json(doc["parameters"].get_document().view()));
            coco_db::create_item(id, get_type(type_id), name, parameters);
        }
        LOG_DEBUG("Retrieved " << get_items().size() << " items");

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

        LOG_DEBUG("Retrieving reactive rules from MongoDB");
        for (const auto &doc : reactive_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_reactive_rule(id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_reactive_rules().size() << " reactive rules");

        if (deliberative_rules_collection.list_indexes().begin() == deliberative_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for deliberative rules collection");
            deliberative_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }

        LOG_DEBUG("Retrieving deliberative rules from MongoDB");
        for (const auto &doc : deliberative_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_deliberative_rule(id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_deliberative_rules().size() << " deliberative rules");
    }

    type &mongo_db::create_type(const std::string &name, const std::string &description, std::unordered_map<std::string, std::unique_ptr<coco_parameter>> &&static_pars, std::unordered_map<std::string, std::unique_ptr<coco_parameter>> &&dynamic_pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto static_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : static_pars)
            static_parameters.append(to_bson(*p.second));
        doc.append(bsoncxx::builder::basic::kvp("static_parameters", static_parameters));
        auto dynamic_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : dynamic_pars)
            dynamic_parameters.append(to_bson(*p.second));
        doc.append(bsoncxx::builder::basic::kvp("dynamic_parameters", dynamic_parameters));
        auto result = types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_type(result->inserted_id().get_oid().value.to_string(), name, description, std::move(static_pars), std::move(dynamic_pars));
        throw std::invalid_argument("Failed to insert type: " + name);
    }

    void mongo_db::set_type_name(type &type, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }
    void mongo_db::set_type_description(type &type, const std::string &description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("description", description))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }
    void mongo_db::add_static_parameter(type &type, std::unique_ptr<coco_parameter> &&par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_parameters", converters.at(par->get_type())->to_bson(*par)))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }
    void mongo_db::remove_static_parameter(type &type, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_parameters", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }
    void mongo_db::add_dynamic_parameter(type &type, std::unique_ptr<coco_parameter> &&par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_parameters", converters.at(par->get_type())->to_bson(*par)))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }
    void mongo_db::remove_dynamic_parameter(type &type, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_parameters", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{type.get_id()})), doc.view());
    }

    void mongo_db::delete_type(const type &type)
    {
        types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic ::kvp("_id", bsoncxx::oid{type.get_id()})));
        coco_db::delete_type(type);
    }

    item &mongo_db::create_item(const type &tp, const std::string &name, const json::json &pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type_id", bsoncxx::oid{tp.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("parameters", bsoncxx::from_json(pars.dump())));
        auto result = items_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_item(result->inserted_id().get_oid().value.to_string(), tp, name, pars);
        throw std::invalid_argument("Failed to insert item: " + name);
    }

    void mongo_db::set_item_name(item &it, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
    }

    void mongo_db::set_item_parameters(item &it, const json::json &pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("parameters", bsoncxx::from_json(pars.dump())))));
        items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
    }

    void mongo_db::delete_item(const item &it)
    {
        items_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})));
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

    rule &mongo_db::create_reactive_rule(const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = reactive_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_reactive_rule(result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert reactive rule: " + name);
    }

    rule &mongo_db::create_deliberative_rule(const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = deliberative_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_deliberative_rule(result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert deliberative rule: " + name);
    }
} // namespace coco
