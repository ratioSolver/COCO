#include "mongo_db.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), parameters_collection(db["parameters"]), items_collection(db["items"]), item_data_collection(db["item_data"]), reactive_rules_collection(db["reactive_rules"]), deliberative_rules_collection(db["deliberative_rules"])
    {
        assert(conn);
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

        if (parameters_collection.list_indexes().begin() == parameters_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for parameters collection");
            parameters_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }

        LOG_DEBUG("Retrieving all parameters from MongoDB");
        std::map<std::string, std::vector<std::string>> super_parameters;
        std::map<std::string, std::vector<std::string>> disjoint_parameters;
        for (const auto &doc : parameters_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            if (doc.find("super_parameters") != doc.end())
            {
                super_parameters[id] = {};
                for (const auto &p : doc["super_parameters"].get_array().value)
                    super_parameters[id].push_back(p.get_oid().value.to_string());
            }
            if (doc.find("disjoint_parameters") != doc.end())
            {
                disjoint_parameters[id] = {};
                for (const auto &p : doc["disjoint_parameters"].get_array().value)
                    disjoint_parameters[id].push_back(p.get_oid().value.to_string());
            }
            auto schema = json::load(bsoncxx::to_json(doc["schema"].get_document().view()));
            coco_db::create_parameter(id, name, description, std::move(schema));
        }
        for (const auto &[id, super_pars] : super_parameters)
            for (const auto &p : super_pars)
                coco_db::add_super_parameter(get_parameter(id), get_parameter(p));
        for (const auto &[id, disjoint_pars] : disjoint_parameters)
            for (const auto &p : disjoint_pars)
                coco_db::add_disjoint_parameter(get_parameter(id), get_parameter(p));
        LOG_DEBUG("Retrieved " << get_parameters().size() << " parameters");

        if (types_collection.list_indexes().begin() == types_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for types collection");
            types_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }

        LOG_DEBUG("Retrieving all types from MongoDB");
        std::map<std::string, std::vector<std::string>> super_types;
        std::map<std::string, std::vector<std::string>> disjoint_types;
        for (const auto &doc : types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            if (doc.find("super_types") != doc.end())
            {
                super_types[id] = {};
                for (const auto &t : doc["super_types"].get_array().value)
                    super_types[id].push_back(t.get_oid().value.to_string());
            }
            if (doc.find("disjoint_types") != doc.end())
            {
                disjoint_types[id] = {};
                for (const auto &t : doc["disjoint_types"].get_array().value)
                    disjoint_types[id].push_back(t.get_oid().value.to_string());
            }
            std::vector<std::reference_wrapper<const parameter>> static_pars;
            if (doc.find("static_parameters") != doc.end())
                for (const auto &p : doc["static_parameters"].get_array().value)
                    static_pars.push_back(get_parameter(p.get_oid().value.to_string()));
            std::vector<std::reference_wrapper<const parameter>> dynamic_pars;
            if (doc.find("dynamic_parameters") != doc.end())
                for (const auto &p : doc["dynamic_parameters"].get_array().value)
                    dynamic_pars.push_back(get_parameter(p.get_oid().value.to_string()));
            coco_db::create_type(id, name, description, std::move(static_pars), std::move(dynamic_pars));
        }
        for (const auto &[id, super_tps] : super_types)
            for (const auto &t : super_tps)
                coco_db::add_super_type(get_type(id), get_type(t));
        for (const auto &[id, disjoint_tps] : disjoint_types)
            for (const auto &t : disjoint_tps)
                coco_db::add_disjoint_type(get_type(id), get_type(t));
        LOG_DEBUG("Retrieved " << get_types().size() << " types");

        if (items_collection.list_indexes().begin() == items_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for items collection");
            items_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        }

        LOG_DEBUG("Retrieving all items from MongoDB");
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

    [[nodiscard]] parameter &mongo_db::create_parameter(const std::string &name, const std::string &description, json::json &&schema)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        doc.append(bsoncxx::builder::basic::kvp("schema", bsoncxx::from_json(schema.dump())));
        auto result = parameters_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_parameter(result->inserted_id().get_oid().value.to_string(), name, description, std::move(schema));
        throw std::invalid_argument("Failed to insert parameter: " + name);
    }

    void mongo_db::set_parameter_name(parameter &par, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::set_parameter_description(parameter &par, const std::string &description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("description", description))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::add_super_parameter(parameter &par, const parameter &super_par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("super_parameters", bsoncxx::oid{super_par.get_id()}))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::remove_super_parameter(parameter &par, const parameter &super_par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("super_parameters", bsoncxx::oid{super_par.get_id()}))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::add_disjoint_parameter(parameter &par, const parameter &disjoint_par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("disjoint_parameters", bsoncxx::oid{disjoint_par.get_id()}))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::remove_disjoint_parameter(parameter &par, const parameter &disjoint_par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("disjoint_parameters", bsoncxx::oid{disjoint_par.get_id()}))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::set_parameter_schema(parameter &par, json::json &&schema)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("schema", bsoncxx::from_json(schema.dump())))));
        parameters_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})), doc.view());
    }
    void mongo_db::delete_parameter(const parameter &par)
    {
        // Remove parameter from all types
        for (const auto &it : get_types())
        {
            if (auto sp_it = std::find_if(it.get().get_static_parameters().begin(), it.get().get_static_parameters().end(), [&par](const auto &p)
                                          { return &p.get() == &par; });
                sp_it != it.get().get_static_parameters().end())
                remove_static_parameter(it, par);
            if (auto dp_it = std::find_if(it.get().get_dynamic_parameters().begin(), it.get().get_dynamic_parameters().end(), [&par](const auto &p)
                                          { return &p.get() == &par; });
                dp_it != it.get().get_dynamic_parameters().end())
                remove_dynamic_parameter(it, par);
        }
        parameters_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{par.get_id()})));
        coco_db::delete_parameter(par);
    }

    type &mongo_db::create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const parameter>> &&static_pars, std::vector<std::reference_wrapper<const parameter>> &&dynamic_pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto static_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : static_pars)
            static_parameters.append(p.get().get_id());
        doc.append(bsoncxx::builder::basic::kvp("static_parameters", static_parameters));
        auto dynamic_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : dynamic_pars)
            dynamic_parameters.append(p.get().get_id());
        doc.append(bsoncxx::builder::basic::kvp("dynamic_parameters", dynamic_parameters));
        auto result = types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_type(result->inserted_id().get_oid().value.to_string(), name, description, std::move(static_pars), std::move(dynamic_pars));
        throw std::invalid_argument("Failed to insert type: " + name);
    }

    void mongo_db::set_type_name(type &tp, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::set_type_description(type &tp, const std::string &description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("description", description))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::add_super_type(type &tp, const type &super_tp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("super_types", bsoncxx::oid{super_tp.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::remove_super_type(type &tp, const type &super_tp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("super_types", bsoncxx::oid{super_tp.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::add_disjoint_type(type &tp, const type &disjoint_tp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("disjoint_types", bsoncxx::oid{disjoint_tp.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::remove_disjoint_type(type &tp, const type &disjoint_tp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("disjoint_types", bsoncxx::oid{disjoint_tp.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::add_static_parameter(type &tp, const parameter &par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_parameters", bsoncxx::oid{par.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::remove_static_parameter(type &tp, const parameter &par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_parameters", bsoncxx::oid{par.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::add_dynamic_parameter(type &tp, const parameter &par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$push", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_parameters", bsoncxx::oid{par.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }
    void mongo_db::remove_dynamic_parameter(type &tp, const parameter &par)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_parameters", bsoncxx::oid{par.get_id()}))));
        types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
    }

    void mongo_db::delete_type(const type &tp)
    {
        // Remove all items of this type
        for (const auto &it : get_items())
            if (&it.get().get_type() == &tp)
                delete_item(it);
        types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic ::kvp("_id", bsoncxx::oid{tp.get_id()})));
        coco_db::delete_type(tp);
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
        item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()})));
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
