#include "mongo_db.h"
#include "coco_core.h"
#include "logging.h"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    mongo_db::mongo_db(const std::string &instance, const std::string &mongodb_instance_uri, const std::string &app, const std::string &mongodb_app_uri) : coco_db(instance, app), app_conn{mongocxx::uri{mongodb_app_uri}}, instance_conn{mongocxx::uri{mongodb_instance_uri}}, app_db(app_conn[app]), instance_db(instance_conn[instance]), instances_collection(app_db["instances"]), users_collection(app_db["users"]), sensor_types_collection(app_db["sensor_types"]), sensors_collection(instance_db["sensors"]), sensor_data_collection(instance_db["sensor_data"]) { LOG("Connecting to `" + mongodb_instance_uri + "` and `" + mongodb_app_uri + "` MongoDB databases.."); }

    void mongo_db::init()
    {
        coco_db::init();
        for ([[maybe_unused]] const auto &c : app_conn.uri().hosts())
            LOG("Connected to MongoDB app server at " + c.name + ":" + std::to_string(c.port));
        for ([[maybe_unused]] const auto &c : instance_conn.uri().hosts())
            LOG("Connected to MongoDB instance server at " + c.name + ":" + std::to_string(c.port));

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
                l = new location{loc_doc["x"].get_double().value, loc_doc["y"].get_double().value};
            }
            auto &s = coco_db::create_sensor(id, name, get_sensor_type(type_id), std::move(l));
            auto last_value = get_last_sensor_value(s);
            coco_db::set_sensor_value(s, std::chrono::system_clock::from_time_t(last_value["timestamp"]), last_value["value"]);
        }
        LOG("Retrieved " << get_sensors().size() << " sensors..");

        LOG("Retrieving " << get_instance() << " instance..");
        auto instance_doc = instances_collection.find_one(bsoncxx::builder::stream::document{} << "name" << get_instance() << bsoncxx::builder::stream::finalize);
        if (instance_doc)
        {
            for (auto user_id : instance_doc->view()["users"].get_array().value)
            {
                auto user_doc = users_collection.find_one(bsoncxx::builder::stream::document{} << "_id" << user_id.get_string() << bsoncxx::builder::stream::finalize);
                if (user_doc)
                {
                    auto id = user_doc->view()["_id"].get_oid().value.to_string();
                    auto admin = user_doc->view()["admin"].get_bool().value;
                    auto first_name = user_doc->view()["first_name"].get_string().value.to_string();
                    auto last_name = user_doc->view()["last_name"].get_string().value.to_string();
                    auto email = user_doc->view()["email"].get_string().value.to_string();
                    auto password = user_doc->view()["password"].get_string().value.to_string();
                    std::vector<std::string> instances;
                    for (auto instance_id : user_doc->view()["instances"].get_array().value)
                        instances.push_back(instance_id.get_string().value.to_string());
                    auto data = json::load(bsoncxx::to_json(user_doc->view()["data"].get_document().value));
                    coco_db::create_user(id, admin, first_name, last_name, email, password, instances, data);
                }
            }
            LOG("Retrieved " << get_users().size() << " users..");
        }
        else
        {
            LOG("Creating new " << get_instance() << " instance..");
            create_instance(get_instance());
        }
    }

    std::string mongo_db::create_instance(const std::string &name, const json::json &data)
    {
        auto i_doc = bsoncxx::builder::basic::document{};
        i_doc.append(bsoncxx::builder::basic::kvp("name", name));
        i_doc.append(bsoncxx::builder::basic::kvp("users", bsoncxx::builder::basic::array{}));
        i_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.to_string())));
        auto result = instances_collection.insert_one(i_doc.view());
        if (result)
            return result->inserted_id().get_oid().value.to_string();
        else
            return {};
    }
    std::vector<instance_ptr> mongo_db::get_instances()
    {
        std::vector<instance_ptr> instances;
        for (auto doc : instances_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            std::vector<std::string> users;
            for (auto user_id : doc["users"].get_array().value)
                users.push_back(user_id.get_string().value.to_string());
            auto data = json::load(bsoncxx::to_json(doc["data"].get_document().value));
            instances.emplace_back(new instance(id, name, users, data));
        }
        return instances;
    }
    std::string mongo_db::create_user(bool admin, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &instances, const json::json &data)
    {
        using bsoncxx::builder::basic::kvp;
        auto u_doc = bsoncxx::builder::basic::document{};
        u_doc.append(kvp("admin", admin));
        u_doc.append(kvp("first_name", first_name));
        u_doc.append(kvp("last_name", last_name));
        u_doc.append(kvp("email", email));
        u_doc.append(kvp("password", password));
        auto instances_doc = bsoncxx::builder::basic::array{};
        for (const auto &instance : instances)
            instances_doc.append(instance);
        u_doc.append(kvp("instances", instances_doc));
        u_doc.append(kvp("data", bsoncxx::from_json(data.to_string())));

        auto result = users_collection.insert_one(u_doc.view());
        if (result)
        {
            auto id = result->inserted_id().get_oid().value.to_string();
            for (const auto &instance : instances)
                if (!instances_collection.update_one(bsoncxx::builder::stream::document{} << "name" << get_instance() << bsoncxx::builder::stream::finalize, bsoncxx::builder::stream::document{} << "$push" << bsoncxx::builder::stream::open_document << "users" << id << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize))
                {
                    LOG_ERR("Failed to add user to instance `" << instance << "`");
                    return {};
                }

            coco_db::create_user(id, admin, first_name, last_name, email, password, instances, data);
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
            auto admin = doc->view()["admin"].get_bool().value;
            auto first_name = doc->view()["first_name"].get_string().value.to_string();
            auto last_name = doc->view()["last_name"].get_string().value.to_string();
            std::vector<std::string> instances;
            for (auto instance_id : doc->view()["instances"].get_array().value)
                instances.push_back(instance_id.get_string().value.to_string());
            auto data = json::load(bsoncxx::to_json(doc->view()["data"].get_document().value));
            return new user(id, admin, first_name, last_name, email, password, instances, data);
        }
        else
            return nullptr;
    }
    std::vector<user_ptr> mongo_db::get_all_users()
    {
        std::vector<user_ptr> users;
        for (auto doc : users_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto admin = doc["admin"].get_bool().value;
            auto first_name = doc["first_name"].get_string().value.to_string();
            auto last_name = doc["last_name"].get_string().value.to_string();
            auto email = doc["email"].get_string().value.to_string();
            auto password = doc["password"].get_string().value.to_string();
            std::vector<std::string> instances;
            for (auto instance_id : doc["instances"].get_array().value)
                instances.push_back(instance_id.get_string().value.to_string());
            auto data = json::load(bsoncxx::to_json(doc["data"].get_document().value));
            users.emplace_back(new user(id, admin, first_name, last_name, email, password, instances, data));
        }
        return users;
    }
    void mongo_db::set_user_admin(user &u, bool admin)
    {
        assert(has_user(u.get_id()));
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "admin" << admin << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_admin(u, admin);
    }
    void mongo_db::set_user_admin(const std::string &id, bool admin)
    {
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "admin" << admin << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result && has_user(id))
            coco_db::set_user_admin(coco_db::get_user(id), admin);
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
    void mongo_db::set_user_instances(user &u, const std::vector<std::string> &instances)
    {
        assert(has_user(u.get_id()));
        for (const auto &instance : u.get_instances())
            if (!instances_collection.update_one(bsoncxx::builder::stream::document{} << "name" << instance << bsoncxx::builder::stream::finalize, bsoncxx::builder::stream::document{} << "$pull" << bsoncxx::builder::stream::open_document << "users" << u.get_id() << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize))
            {
                LOG_ERR("Failed to remove user from instance `" << instance << "`");
                return;
            }
        for (const auto &instance : instances)
            if (!instances_collection.update_one(bsoncxx::builder::stream::document{} << "name" << instance << bsoncxx::builder::stream::finalize, bsoncxx::builder::stream::document{} << "$push" << bsoncxx::builder::stream::open_document << "users" << u.get_id() << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize))
            {
                LOG_ERR("Failed to add user to instance `" << instance << "`");
                return;
            }
        auto instances_doc = bsoncxx::builder::basic::array{};
        for (const auto &instance : instances)
            instances_doc.append(instance);
        auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{u.get_id()}} << bsoncxx::builder::stream::finalize,
                                                  bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "instances" << instances_doc << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
        if (result)
            coco_db::set_user_instances(u, instances);
    }
    void mongo_db::set_user_instances(const std::string &id, const std::vector<std::string> &instances)
    {
        auto user_doc = users_collection.find_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize);
        if (user_doc)
        {
            auto user_instances = user_doc->view()["instances"].get_array().value;
            for (const auto &instance : user_instances)
                if (!instances_collection.update_one(bsoncxx::builder::stream::document{} << "name" << instance.get_string() << bsoncxx::builder::stream::finalize, bsoncxx::builder::stream::document{} << "$pull" << bsoncxx::builder::stream::open_document << "users" << id << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize))
                {
                    LOG_ERR("Failed to remove user from instance `" << instance.get_string().value.to_string() << "`");
                    return;
                }
            for (const auto &instance : instances)
                if (!instances_collection.update_one(bsoncxx::builder::stream::document{} << "name" << instance << bsoncxx::builder::stream::finalize, bsoncxx::builder::stream::document{} << "$push" << bsoncxx::builder::stream::open_document << "users" << id << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize))
                {
                    LOG_ERR("Failed to add user to instance `" << instance << "`");
                    return;
                }
            auto instances_doc = bsoncxx::builder::basic::array{};
            for (const auto &instance : instances)
                instances_doc.append(instance);
            auto result = users_collection.update_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{bsoncxx::stdx::string_view{id}} << bsoncxx::builder::stream::finalize,
                                                      bsoncxx::builder::stream::document{} << "$set" << bsoncxx::builder::stream::open_document << "instances" << instances_doc << bsoncxx::builder::stream::close_document << bsoncxx::builder::stream::finalize);
            if (result && has_user(id))
                coco_db::set_user_instances(coco_db::get_user(id), instances);
        }
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
    json::json mongo_db::get_sensor_values(sensor &s, const std::chrono::system_clock::time_point &start, const std::chrono::system_clock::time_point &end)
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
    void mongo_db::set_sensor_value(sensor &s, const std::chrono::system_clock::time_point &time, const json::json &val)
    {
        auto result = sensor_data_collection.insert_one(bsoncxx::builder::stream::document{} << "sensor_id" << bsoncxx::oid{bsoncxx::stdx::string_view{s.get_id()}} << "timestamp" << bsoncxx::types::b_date{time} << "value" << bsoncxx::from_json(val.to_string()) << bsoncxx::builder::stream::finalize);
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
        app_db.drop();
        instance_db.drop();
        coco_db::drop();
        instances_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize);
        users_collection.create_index(bsoncxx::builder::stream::document{} << "email" << 1 << "password" << 1 << bsoncxx::builder::stream::finalize);
        sensor_data_collection.create_index(bsoncxx::builder::stream::document{} << "sensor_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize);
    }
} // namespace coco
