#include "mongo_db.h"
#include "coco_core.h"
#include "logging.h"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(const std::string &root, const std::string &mongodb_uri) : coco_db(root), conn{mongocxx::uri{mongodb_uri}}, db(conn["coco"]), root_db(conn[root]), users_collection(db["users"]), sensor_types_collection(db["sensor_types"]), sensors_collection(root_db["coco"]), sensor_data_collection(root_db["sensor_data"]) { LOG("Connecting to `" + mongodb_uri + "` MongoDB database.."); }

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

        LOG("Retrieving all " << get_root() << " sensors..");
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
                l = new location{loc_doc["x"].get_double().value, loc_doc["y"].get_double().value};
            }
            coco_db::create_sensor(id, name, get_sensor_type(type_id), std::move(l));
        }
        LOG("Retrieved " << get_sensors().size() << " sensors..");
    }

    std::string mongo_db::create_user(const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &roots, const json::json &data)
    {
        auto u_doc = bsoncxx::builder::basic::document{};
        u_doc.append(bsoncxx::builder::basic::kvp("first_name", first_name));
        u_doc.append(bsoncxx::builder::basic::kvp("last_name", last_name));
        u_doc.append(bsoncxx::builder::basic::kvp("email", email));
        u_doc.append(bsoncxx::builder::basic::kvp("password", password));
        auto roots_doc = bsoncxx::builder::basic::array{};
        for (const auto &root : roots)
            roots_doc.append(root);
        u_doc.append(bsoncxx::builder::basic::kvp("roots", roots_doc));
        u_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.to_string())));

        auto result = users_collection.insert_one(u_doc.view());
        if (result)
        {
            auto id = result->inserted_id().get_oid().value.to_string();
            coco_db::create_user(id, first_name, last_name, email, password, roots, data);
            return id;
        }
        else
            return {};
    }
    user_ptr mongo_db::get_user(const std::string &email, const std::string &password)
    {
        auto doc = users_collection.find_one(bsoncxx::builder::stream::document{} << "email" << email << "password" << password << bsoncxx::builder::stream::finalize);
        if (doc)
        {
            auto id = doc->view()["_id"].get_oid().value.to_string();
            auto first_name = doc->view()["first_name"].get_string().value.to_string();
            auto last_name = doc->view()["last_name"].get_string().value.to_string();
            auto roots_doc = doc->view()["roots"].get_array().value;
            std::vector<std::string> roots;
            for (auto root : roots_doc)
                roots.push_back(root.get_string().value.to_string());
            auto data = json::load(bsoncxx::to_json(doc->view()["data"].get_document().value));
            return new user(id, first_name, last_name, email, password, roots, data);
        }
        else
            return nullptr;
    }
    std::vector<user_ptr> mongo_db::get_users()
    {
        std::vector<user_ptr> users;
        for (auto doc : users_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto first_name = doc["first_name"].get_string().value.to_string();
            auto last_name = doc["last_name"].get_string().value.to_string();
            auto email = doc["email"].get_string().value.to_string();
            auto password = doc["password"].get_string().value.to_string();
            auto roots_doc = doc["roots"].get_array().value;
            std::vector<std::string> roots;
            for (auto root : roots_doc)
                roots.push_back(root.get_string().value.to_string());
            auto data = json::load(bsoncxx::to_json(doc["data"].get_document().value));
            users.emplace_back(new user(id, first_name, last_name, email, password, roots, data));
        }
        return users;
    }
    void mongo_db::set_user_first_name(user &u, const std::string &first_name)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "first_name" << first_name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_first_name(u, first_name);
    }
    void mongo_db::set_user_first_name(const std::string &id, const std::string &first_name)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "first_name" << first_name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_first_name(coco_db::get_user(id), first_name);
    }
    void mongo_db::set_user_last_name(user &u, const std::string &last_name)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "last_name" << last_name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_last_name(u, last_name);
    }
    void mongo_db::set_user_last_name(const std::string &id, const std::string &last_name)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "last_name" << last_name << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_last_name(coco_db::get_user(id), last_name);
    }
    void mongo_db::set_user_email(user &u, const std::string &email)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "email" << email << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_email(u, email);
    }
    void mongo_db::set_user_email(const std::string &id, const std::string &email)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "email" << email << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_email(coco_db::get_user(id), email);
    }
    void mongo_db::set_user_password(user &u, const std::string &password)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "password" << password << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_password(u, password);
    }
    void mongo_db::set_user_password(const std::string &id, const std::string &password)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "password" << password << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_password(coco_db::get_user(id), password);
    }
    void mongo_db::set_user_roots(user &u, const std::vector<std::string> &roots)
    {
        assert(has_user(u.get_id()));
        auto roots_doc = bsoncxx::builder::basic::array{};
        for (const auto &root : roots)
            roots_doc.append(root);
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "roots" << roots_doc << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_roots(u, roots);
    }
    void mongo_db::set_user_roots(const std::string &id, const std::vector<std::string> &roots)
    {
        auto roots_doc = bsoncxx::builder::basic::array{};
        for (const auto &root : roots)
            roots_doc.append(root);
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "roots" << roots_doc << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_roots(coco_db::get_user(id), roots);
    }
    void mongo_db::set_user_data(user &u, const json::json &data)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "data" << bsoncxx::from_json(data.to_string()) << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_data(u, data);
    }
    void mongo_db::set_user_data(const std::string &id, const json::json &data)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "data" << bsoncxx::from_json(data.to_string()) << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_data(coco_db::get_user(id), data);
    }
    void mongo_db::delete_user(user &u)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::delete_user(u);
    }
    void mongo_db::delete_user(const std::string &id)
    {
        auto result = users_collection.delete_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::delete_user(coco_db::get_user(id));
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

    json::json mongo_db::get_sensor_values(sensor &s, const std::chrono::milliseconds::rep &start, const std::chrono::milliseconds::rep &end)
    {
        auto cursor = sensor_data_collection.find(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << "timestamp" << bsoncxx::builder::stream::open_document << "$gte" << bsoncxx::types::b_date{std::chrono::milliseconds{start}} << "$lte" << bsoncxx::types::b_date{std::chrono::milliseconds{end}} << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        json::json data(json::json_type::array);
        for (auto &&doc : cursor)
            data.push_back(json::load(bsoncxx::to_json(doc["value"].get_document().view())));
        return data;
    }
    void mongo_db::set_sensor_value(sensor &s, const std::chrono::milliseconds::rep &time, const json::json &val)
    {
        auto result = sensor_data_collection.insert_one(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << "timestamp" << bsoncxx::types::b_date{std::chrono::milliseconds{time}} << "value" << bsoncxx::from_json(val.to_string()) << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_sensor_value(s, time, val);
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
        root_db.drop();
        coco_db::drop();
        users_collection.create_index(bsoncxx::builder::stream::document{} << "email" << 1 << "password" << 1 << bsoncxx::builder::stream::finalize);
        sensor_data_collection.create_index(bsoncxx::builder::stream::document{} << "sensor_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize);
    }
} // namespace coco
