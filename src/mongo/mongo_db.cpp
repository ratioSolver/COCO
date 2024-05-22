#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cassert>
#include "mongo_db.hpp"

namespace coco
{
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), items_collection(db["items"]), sensor_data_collection(db["sensor_data"]), reactive_rules_collection(db["reactive_rules"]), deliberative_rules_collection(db["deliberative_rules"]) { assert(conn); }

    type &mongo_db::create_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto static_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : static_pars)
            static_parameters.append(to_bson(*p));
        doc.append(bsoncxx::builder::basic::kvp("static_parameters", static_parameters));
        auto dynamic_parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : dynamic_pars)
            dynamic_parameters.append(to_bson(*p));
        doc.append(bsoncxx::builder::basic::kvp("dynamic_parameters", dynamic_parameters));
        auto result = types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_type(result->inserted_id().get_oid().value.to_string(), name, description, std::move(static_pars), std::move(dynamic_pars));
        throw std::invalid_argument("Failed to insert type: " + name);
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

    void mongo_db::add_data(const item &it, const std::chrono::system_clock::time_point &timestamp, const json::json &data)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        auto result = sensor_data_collection.insert_one(doc.view());
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

    bsoncxx::builder::basic::document mongo_db::to_bson(const parameter &p)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", p.get_name()));
        switch (p.get_type())
        {
        case parameter_type::Integer:
        {
            const auto &ip = static_cast<const integer_parameter &>(p);
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::Integer)));
            if (ip.get_min() != std::numeric_limits<long>::min())
                doc.append(bsoncxx::builder::basic::kvp("min", ip.get_min()));
            if (ip.get_max() != std::numeric_limits<long>::max())
                doc.append(bsoncxx::builder::basic::kvp("max", ip.get_max()));
            break;
        }
        case parameter_type::Real:
        {
            const auto &fp = static_cast<const real_parameter &>(p);
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::Real)));
            if (fp.get_min() != std::numeric_limits<double>::min())
                doc.append(bsoncxx::builder::basic::kvp("min", fp.get_min()));
            if (fp.get_max() != std::numeric_limits<double>::max())
                doc.append(bsoncxx::builder::basic::kvp("max", fp.get_max()));
            break;
        }
        case parameter_type::Boolean:
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::Boolean)));
            break;
        case parameter_type::Symbol:
        {
            const auto &sp = static_cast<const symbol_parameter &>(p);
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::Symbol)));
            if (!sp.get_symbols().empty())
            {
                bsoncxx::v_noabi::builder::basic::array values;
                for (const auto &value : sp.get_symbols())
                    values.append(value);
                doc.append(bsoncxx::builder::basic::kvp("values", values));
            }
            break;
        }
        case parameter_type::String:
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::String)));
            break;
        case parameter_type::Array:
        {
            const auto &ap = static_cast<const array_parameter &>(p);
            doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(parameter_type::Array)));
            doc.append(bsoncxx::builder::basic::kvp("array_type", to_bson(ap.as_array_type())));
            bsoncxx::v_noabi::builder::basic::array dimensions;
            for (const auto &dim : ap.get_shape())
                dimensions.append(dim);
            doc.append(bsoncxx::builder::basic::kvp("dimensions", dimensions));
            break;
        }
        default:
            assert(false);
        }
        return doc;
    }
} // namespace coco
