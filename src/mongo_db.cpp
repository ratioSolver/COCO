#include "mongo_db.h"
#include "coco.h"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>

namespace coco
{
    mongo_db::mongo_db(const std::string &root, const std::string &mongodb_uri) : coco_db(root), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["coco"]), sensor_data_collection(db["sensor_data"]) {}

    std::string mongo_db::create_sensor_type(const std::string &name, const std::string &description)
    {
        auto result = sensor_types_collection.insert_one(bsoncxx::builder::stream::document{} << "name" << name << "description" << description << bsoncxx::builder::stream::finalize);
        if (result)
        {
            auto id = result.value().inserted_id().get_string().value.to_string();
            coco_db::create_sensor_type(id, name, description);
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
        auto s_doc = bsoncxx::builder::stream::document{} << "type_id" << bsoncxx::oid{bsoncxx::stdx::string_view{type.get_id()}};
        if (l)
            s_doc << "location" << bsoncxx::builder::stream::open_document << "x" << l->x << "y" << l->y << bsoncxx::builder::stream::close_document;

        auto result = sensors_collection.insert_one(s_doc << bsoncxx::builder::stream::finalize);
        if (result)
        {
            auto id = result.value().inserted_id().get_string().value.to_string();
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
    void mongo_db::delete_sensor(const std::string &id)
    {
        auto result = sensors_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_sensor(id);
    }
} // namespace coco
