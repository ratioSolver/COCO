#include "mongo_db.h"
#include "coco_core.h"
#include "logging.h"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(const std::string &name, const std::string &mongodb_uri) : coco_db(name), conn{mongocxx::uri{mongodb_uri}}, db(conn[name]), instances_collection(db["instances"]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["sensors"]), sensor_data_collection(db["sensor_data"]) { LOG("Connecting to `" + mongodb_uri + "` MongoDB databases.."); }

    void mongo_db::init()
    {
        coco_db::init();
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

        LOG("Retrieving all sensor types..");
        for (auto doc : sensor_types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            std::map<std::string, parameter_type> parameters;
            for (auto param : doc["parameters"].get_array().value)
            {
                auto param_doc = param.get_document().value;
                auto param_name = param_doc["name"].get_string().value.to_string();
                auto param_type = param_doc["type"].get_int32().value;
                parameters.emplace(param_name, static_cast<parameter_type>(param_type));
            }
            coco_db::create_sensor_type(id, name, description, parameters);
        }
        LOG("Retrieved " << get_sensor_types().size() << " sensor types..");

        LOG("Retrieving all " << get_instance() << " sensors..");
        for (auto &doc : sensors_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto type_id = doc["type_id"].get_oid().value.to_string();

            auto loc = doc.find("location");
            location_ptr l;
            if (loc != doc.end())
            {
                auto loc_doc = loc->get_document().value;
                l = std::make_unique<location>(loc_doc["x"].get_double().value, loc_doc["y"].get_double().value);
            }
            auto &s = coco_db::create_sensor(id, name, get_sensor_type(type_id), std::move(l));
            auto last_value = get_last_sensor_value(s);
            coco_db::set_sensor_data(s, std::chrono::system_clock::from_time_t(last_value["timestamp"]), last_value["value"]);
        }
        LOG("Retrieved " << get_sensors().size() << " sensors..");

        LOG("Retrieving `" << get_instance() << "` instance..");
        auto instance_doc = instances_collection.find_one(bsoncxx::builder::stream::document{} << "name" << get_name() << bsoncxx::builder::stream::finalize);
        if (!instance_doc)
        {
            LOG_WARN("Creating new `" << get_instance() << "` instance..");
            auto instance_id = create_instance(get_name());
        }
    }

    std::string mongo_db::create_instance(const std::string &name, const json::json &data)
    {
        auto i_doc = bsoncxx::builder::basic::document{};
        i_doc.append(bsoncxx::builder::basic::kvp("name", name));
        i_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.to_string())));
        auto result = instances_collection.insert_one(i_doc.view());
        if (result)
            return result->inserted_id().get_oid().value.to_string();
        else
            return {};
    }

    std::string mongo_db::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameters)
    {
        auto s_doc = bsoncxx::builder::basic::document{};
        s_doc.append(bsoncxx::builder::basic::kvp("name", name));
        s_doc.append(bsoncxx::builder::basic::kvp("description", description));
        auto param_types = bsoncxx::builder::basic::array{};
        for (const auto &param : parameters)
        {
            auto param_doc = bsoncxx::builder::basic::document{};
            param_doc.append(bsoncxx::builder::basic::kvp("name", param.first));
            param_doc.append(bsoncxx::builder::basic::kvp("type", static_cast<int>(param.second)));
            param_types.append(param_doc);
        }
        s_doc.append(bsoncxx::builder::basic::kvp("parameters", param_types));
        auto result = sensor_types_collection.insert_one(s_doc.view());
        if (result)
        {
            auto id = result->inserted_id().get_oid().value.to_string();
            coco_db::create_sensor_type(id, name, description, parameters);
            return id;
        }
        else
            return {};
    }
    void mongo_db::set_sensor_type_name(sensor_type &st, const std::string &name)
    {
        auto result = sensor_types_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{st.get_id()}} << bsoncxx::builder::stream::finalize,
                                                         bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "name" << name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_type_name(st, name);
    }
    void mongo_db::set_sensor_type_description(sensor_type &st, const std::string &description)
    {
        auto result = sensor_types_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{st.get_id()}} << bsoncxx::builder::stream::finalize,
                                                         bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "description" << description << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_type_description(st, description);
    }
    void mongo_db::delete_sensor_type(sensor_type &st)
    {
        auto result = sensor_types_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{st.get_id()}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_sensor_type(st);
    }

    std::string mongo_db::create_sensor(const std::string &name, sensor_type &type, location_ptr l)
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
    void mongo_db::set_sensor_name(sensor &s, const std::string &name)
    {
        auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << bsoncxx::builder::stream::finalize,
                                                    bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "name" << name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_name(s, name);
    }
    void mongo_db::set_sensor_location(sensor &s, location_ptr l)
    {
        if (l)
        {
            auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << bsoncxx::builder::stream::finalize,
                                                        bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "location" << bsoncxx::builder::stream::open_document << "x" << l->x << "y" << l->y << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
            if (result)
                coco_db::set_sensor_location(s, std::move(l));
        }
        else
        {
            auto result = sensors_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << bsoncxx::builder::stream::finalize,
                                                        bsoncxx::builder::stream::document{} << "$unset" << bsoncxx::builder::stream::open_document << "location"
                                                                                             << "" << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
            if (result)
                coco_db::set_sensor_location(s, std::move(l));
        }
    }

    json::json mongo_db::get_last_sensor_value(sensor &s)
    {
        if (s.has_value())
        {
            json::json data;
            data["timestamp"] = std::chrono::system_clock::to_time_t(s.get_last_update());
            data["value"] = s.get_value();
            return data;
        }
        auto cursor = sensor_data_collection.find(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  mongocxx::options::find{}.sort(bsoncxx::builder::stream::document{} << "timestamp" << -1 << bsoncxx::builder::stream::finalize).limit(1));
        json::json data;
        for (auto &&doc : cursor)
        {
            data["timestamp"] = doc["timestamp"].get_date().value.count();
            data["value"] = json::load(bsoncxx::to_json(doc["value"].get_document().value));
        }
        return data;
    }
    json::json mongo_db::get_sensor_data(sensor &s, const std::chrono::system_clock::time_point &start, const std::chrono::system_clock::time_point &end)
    {
        auto cursor = sensor_data_collection.find(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << "timestamp" << bsoncxx::builder::stream::open_document << "$gte" << bsoncxx::types::b_date{start} << "$lte" << bsoncxx::types::b_date{end} << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize,
                                                  mongocxx::options::find{}.sort(bsoncxx::builder::stream::document{} << "timestamp" << 1 << bsoncxx::builder::stream::finalize));
        json::json data(json::json_type::array);
        for (auto &&doc : cursor)
        {
            json::json j;
            j["timestamp"] = doc["timestamp"].get_date().value.count();
            j["value"] = json::load(bsoncxx::to_json(doc["value"].get_document().value));
            data.push_back(std::move(j));
        }
        return data;
    }
    void mongo_db::set_sensor_data(sensor &s, const std::chrono::system_clock::time_point &time, const json::json &val)
    {
        coco_db::set_sensor_data(s, time, val);
        auto result = sensor_data_collection.insert_one(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << "timestamp" << bsoncxx::types::b_date{time} << "value" << bsoncxx::from_json(val.to_string()) << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_data(s, time, val);
    }
    void mongo_db::delete_sensor(sensor &s)
    {
        auto result = sensors_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_sensor(s);
    }

    void mongo_db::drop()
    {
        LOG_WARN("Dropping database..");
        db.drop();
        coco_db::drop();
        instances_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        sensor_data_collection.create_index(bsoncxx::builder::stream::document{} << "sensor_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize);
    }
} // namespace coco
