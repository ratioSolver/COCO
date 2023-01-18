#include "mongo_db.h"
#include "coco_core.h"
#include "logging.h"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace coco
{
    mongo_db::mongo_db(const std::string &root, const std::string &mongodb_uri) : coco_db(root), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["coco"]), sensor_data_collection(db["sensor_data"]) { LOG("Connecting to `" + root + "` MongoDB database.."); }

    void mongo_db::init()
    {
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
        {
            LOG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));
        }
        LOG("Retrieving all sensor types..");
        for (auto doc : sensor_types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            std::map<std::string, parameter_type> parameter_types;
            for (auto param : doc["parameter_types"].get_array().value)
            {
                auto param_doc = param.get_document().value;
                auto param_name = param_doc["name"].get_string().value.to_string();
                auto param_type = param_doc["type"].get_int32().value;
                parameter_types.emplace(param_name, static_cast<parameter_type>(param_type));
            }
            coco_db::create_sensor_type(id, name, description, parameter_types);
        }
        LOG("Retrieved " + std::to_string(get_all_sensor_types().size()) + " sensor types..");

        LOG("Retrieving all sensors..");
        for (auto doc : sensors_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto type_id = doc["type_id"].get_oid().value.to_string();

            auto loc = doc.find("location");
            std::unique_ptr<location> l;
            if (loc != doc.end())
            {
                auto loc_doc = loc->get_document().value;
                auto x = loc_doc["x"].get_double().value;
                auto y = loc_doc["y"].get_double().value;
                l = std::make_unique<location>();
                l->x = x;
                l->y = y;
            }
            coco_db::create_sensor(id, name, get_sensor_type(type_id), std::move(l));
        }
        LOG("Retrieved " + std::to_string(get_all_sensors().size()) + " sensors..");
    }
    std::string mongo_db::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        auto s_doc = bsoncxx::builder::basic::document{};
        s_doc.append(bsoncxx::builder::basic::kvp("name", name));
        s_doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto param_types = bsoncxx::builder::basic::array{};
        for (const auto &param : parameter_types)
        {
            auto param_doc = bsoncxx::builder::basic::document{};
            param_doc.append(bsoncxx::builder::basic::kvp("name", param.first));
            param_doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int32_t>(param.second)));
            param_types.append(param_doc);
        }
        s_doc.append(bsoncxx::builder::basic::kvp("parameter_types", param_types));
        auto result = sensor_types_collection.insert_one(s_doc.view());
        if (result)
        {
            auto id = result->inserted_id().get_oid().value.to_string();
            coco_db::create_sensor_type(id, name, description, parameter_types);
            return id;
        }
        else
            return {};
    }
    void mongo_db::set_sensor_type_name(const std::string &id, const std::string &name)
    {
        auto result = sensor_types_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                         bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "name" << name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_type_name(id, name);
    }
    void mongo_db::set_sensor_type_description(const std::string &id, const std::string &description)
    {
        auto result = sensor_types_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                         bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "description" << description << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_type_description(id, description);
    }
    void mongo_db::delete_sensor_type(const std::string &id)
    {
        auto result = sensor_types_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_sensor_type(id);
    }

    std::string mongo_db::create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l)
    {
        auto s_doc = bsoncxx::builder::basic::document{};
        s_doc.append(bsoncxx::builder::basic::kvp("name", name));
        s_doc.append(bsoncxx::builder::basic::kvp("type_id", bsoncxx::oid{bsoncxx::stdx::string_view{type.get_id()}}));
        if (l)
            s_doc.append(bsoncxx::builder::basic::kvp("location", [&l](bsoncxx::builder::basic ::sub_document subdoc)
                                                      { subdoc.append(bsoncxx::builder::basic::kvp("x", l->x), bsoncxx::builder::basic::kvp("y", l->y)); }));

        auto result = sensors_collection.insert_one(s_doc.view());
        if (result)
        {
            auto id = result->inserted_id().get_oid().value.to_string();
            coco_db::create_sensor(id, name, type, std::move(l));
            return id;
        }
        else
            return {};
    }
    void mongo_db::set_sensor_name(const std::string &id, const std::string &name)
    {
        auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                    bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "name" << name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_name(id, name);
    }
    void mongo_db::set_sensor_location(const std::string &id, std::unique_ptr<location> l)
    {
        if (l)
        {
            auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                        bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "location" << bsoncxx::builder::stream::open_document << "x" << l->x << "y" << l->y << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
            if (result)
                coco_db::set_sensor_location(id, std::move(l));
        }
        else
        {
            auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                        bsoncxx::builder::stream::document{} << "$unset" << bsoncxx::builder::stream::open_document << "location"
                                                                                             << "" << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
            if (result)
                coco_db::set_sensor_location(id, std::move(l));
        }
    }
    void mongo_db::set_sensor_value(const std::string &id, const std::chrono::milliseconds::rep &time, const json::json &val)
    {
        auto result = sensor_data_collection.insert_one(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << "timestamp" << bsoncxx::types::b_date{std::chrono::milliseconds{time}} << "value" << bsoncxx::from_json(val.dump()) << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_value(id, time, val);
    }
    void mongo_db::delete_sensor(const std::string &id)
    {
        auto result = sensors_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_sensor(id);
    }

    void mongo_db::drop()
    {
        db.drop();
        coco_db::drop();
        sensor_data_collection.create_index(bsoncxx::builder::stream::document{} << "sensor_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize);
    }
} // namespace coco
