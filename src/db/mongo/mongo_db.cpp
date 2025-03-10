#include "mongo_db.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(json::json &&cnfg, const std::string &mongodb_uri) noexcept : coco_db(std::move(cnfg)), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"])
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

    void mongo_db::create_type(std::string_view name, const json::json &data, const json::json &static_props, const json::json &dynamic_props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("_id", name.data()));
        if (!data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        if (!static_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("static_properties", bsoncxx::from_json(static_props.dump())));
        if (!dynamic_props.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("dynamic_properties", bsoncxx::from_json(dynamic_props.dump())));
        if (!types_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert type: " + std::string(name));
    }
} // namespace coco
