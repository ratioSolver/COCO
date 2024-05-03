#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>
#include "mongo_db.hpp"

namespace coco
{
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["sensors"]), sensor_data_collection(db["sensor_data"]) {}

    sensor_type &mongo_db::create_sensor_type(const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto parameters = bsoncxx::builder::basic::array{};
        for (const auto &p : pars)
            parameters.append(to_bson(*p));
        doc.append(bsoncxx::builder::basic::kvp("parameters", parameters));
        auto result = sensor_types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_sensor_type(result->inserted_id().get_oid().value.to_string(), name, description, std::move(pars));
        throw std::invalid_argument("Failed to insert sensor type: " + name);
    }

    bsoncxx::v_noabi::builder::basic::document mongo_db::to_bson(const parameter &p)
    {
        bsoncxx::v_noabi::builder::basic::document doc;
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
            doc.append(bsoncxx::builder::basic::kvp("array_type", to_bson(ap.get_array_type())));
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
