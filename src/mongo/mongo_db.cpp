#include "mongo_db.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cmath>
#include <cassert>
#ifdef ENABLE_AUTH
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <iomanip>
#endif

namespace coco
{
#ifdef ENABLE_AUTH
    std::string encode_password(const std::string &password, const std::string &salt)
    {
        int iterations = 10000;
        unsigned char hash[32];
        if (PKCS5_PBKDF2_HMAC(password.c_str(), password.size(), reinterpret_cast<const unsigned char *>(salt.c_str()), salt.size(), iterations, EVP_sha256(), sizeof(hash), hash) == 0)
            throw std::runtime_error("PKCS5_PBKDF2_HMAC failed");

        std::stringstream hash_stream;
        for (unsigned char c : hash)
            hash_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        return hash_stream.str();
    }

    std::pair<std::string, std::string> encode_password(const std::string &password)
    {
        unsigned char salt[16];
        RAND_bytes(salt, sizeof(salt));
        std::stringstream salt_stream;
        for (unsigned char c : salt)
            salt_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
        return {salt_stream.str(), encode_password(password, salt_stream.str())};
    }
#endif

#ifdef ENABLE_AUTH
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), users_collection(db["users"]), types_collection(db["types"]), items_collection(db["items"]), item_data_collection(db["item_data"]), reactive_rules_collection(db["reactive_rules"]), deliberative_rules_collection(db["deliberative_rules"])
#else
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), types_collection(db["types"]), items_collection(db["items"]), item_data_collection(db["item_data"]), reactive_rules_collection(db["reactive_rules"]), deliberative_rules_collection(db["deliberative_rules"])
#endif
    {
        assert(conn);
        for ([[maybe_unused]] const auto &c : conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

#ifdef ENABLE_AUTH
        if (users_collection.list_indexes().begin() == users_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for users collection");
            users_collection.create_index(bsoncxx::builder::stream::document{} << "username" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));

            LOG_WARN("Creating default admin user. Please change the password immediately.");
            create_user("admin", "admin", {0});
        }
#endif

        if (types_collection.list_indexes().begin() == types_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for types collection");
            types_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (item_data_collection.list_indexes().begin() == item_data_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for item data collection");
            item_data_collection.create_index(bsoncxx::builder::stream::document{} << "item_id" << 1 << "timestamp" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (reactive_rules_collection.list_indexes().begin() == reactive_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for reactive rules collection");
            reactive_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
        if (deliberative_rules_collection.list_indexes().begin() == deliberative_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for deliberative rules collection");
            deliberative_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    void mongo_db::init(coco_core &cc)
    {
        coco_db::init(cc);
        LOG_DEBUG("Retrieving all types from MongoDB");
        std::vector<bsoncxx::document::value> types;
        for (const auto &doc : types_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto description = doc["description"].get_string().value.to_string();
            auto properties = json::load(bsoncxx::to_json(doc["properties"].get_document().view()));
            coco_db::create_type(cc, id, name, description, std::move(properties));
            types.push_back(bsoncxx::document::value{doc});
        }
        for (const auto &doc : types)
        {
            const auto id = doc["_id"].get_oid().value.to_string();
            auto &tp = get_type(id);

            if (doc.find("parents") != doc.end())
                for (const auto &p : doc["parents"].get_array().value)
                    add_parent(tp, get_type(p.get_oid().value.to_string()));
            if (doc.find("static_properties") != doc.end())
                for (const auto &p : doc["static_properties"].get_document().value)
                {
                    auto prop = make_property(cc, p.key().to_string(), json::load(bsoncxx::to_json(p.get_document().value)));
                    add_static_property(tp, std::move(prop));
                }
            if (doc.find("dynamic_properties") != doc.end())
                for (const auto &p : doc["dynamic_properties"].get_document().value)
                {
                    auto prop = make_property(cc, p.key().to_string(), json::load(bsoncxx::to_json(p.get_document().value)));
                    add_dynamic_property(tp, std::move(prop));
                }
        }
        LOG_DEBUG("Retrieved " << types.size() << " types");

        LOG_DEBUG("Retrieving reactive rules from MongoDB");
        for (const auto &doc : reactive_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_reactive_rule(cc, id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_reactive_rules().size() << " reactive rules");

        LOG_DEBUG("Retrieving deliberative rules from MongoDB");
        for (const auto &doc : deliberative_rules_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto name = doc["name"].get_string().value.to_string();
            auto content = doc["content"].get_string().value.to_string();
            coco_db::create_deliberative_rule(cc, id, name, content);
        }
        LOG_DEBUG("Retrieved " << get_deliberative_rules().size() << " deliberative rules");

        LOG_DEBUG("Retrieving all items from MongoDB");
        for (const auto &doc : items_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto type_id = doc["type_id"].get_oid().value.to_string();
            auto properties = json::load(bsoncxx::to_json(doc["properties"].get_document().view()));
            auto value = json::load(bsoncxx::to_json(doc["value"]["data"].get_document().view()));
            auto timestamp = doc["value"]["timestamp"].get_date().to_int64();
            coco_db::create_item(cc, id, get_type(type_id), std::move(properties), value, std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp)));
        }
        LOG_DEBUG("Retrieved " << get_items().size() << " items");
    }

#ifdef ENABLE_AUTH
    coco_user mongo_db::create_user(const std::string &username, const std::string &password, std::set<int> &&roles, json::json &&data)
    {
        auto [salt, hash] = encode_password(password);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("username", username));
        doc.append(bsoncxx::builder::basic::kvp("salt", salt));
        doc.append(bsoncxx::builder::basic::kvp("password", hash));
        auto roles_array = bsoncxx::builder::basic::array{};
        for (const auto &r : roles)
            roles_array.append(r);
        doc.append(bsoncxx::builder::basic::kvp("roles", roles_array));
        if (!data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));

        if (auto result = users_collection.insert_one(doc.view()); result)
            return coco_user(result->inserted_id().get_oid().value.to_string(), username, std::move(roles), std::move(data));
        throw std::runtime_error("Failed to insert user: " + username);
    }

    std::vector<coco_user> mongo_db::get_users()
    {
        std::vector<coco_user> users;
        for (const auto &doc : users_collection.find({}))
        {
            auto id = doc["_id"].get_oid().value.to_string();
            auto username = doc["username"].get_string().value.to_string();
            std::set<int> roles;
            for (const auto &r : doc["roles"].get_array().value)
                roles.insert(r.get_int32().value);
            users.push_back(coco_user(id, username, std::move(roles)));
        }
        return users;
    }

    coco_user mongo_db::get_user(const std::string &username, const std::string &password)
    {
        if (auto doc = users_collection.find_one(bsoncxx::builder::stream::document{} << "username" << username << bsoncxx::builder::stream::finalize); doc)
        {
            if (auto salt = doc->view()["salt"].get_string().value.to_string(); encode_password(password, salt) == doc->view()["password"].get_string().value.to_string())
            {
                std::set<int> roles;
                for (const auto &r : doc->view()["roles"].get_array().value)
                    roles.insert(r.get_int32().value);
                return coco_user(doc->view()["_id"].get_oid().value.to_string(), username, std::move(roles));
            }
            throw std::runtime_error("Invalid password for user: " + username);
        }
        throw std::runtime_error("Failed to get user: " + username);
    }

    coco_user mongo_db::get_user(const std::string &id)
    {
        if (auto doc = users_collection.find_one(bsoncxx::builder::stream::document{} << "_id" << bsoncxx::oid{id} << bsoncxx::builder::stream::finalize); doc)
        {
            auto username = doc->view()["username"].get_string().value.to_string();
            std::set<int> roles;
            for (const auto &r : doc->view()["roles"].get_array().value)
                roles.insert(r.get_int32().value);
            return coco_user(id, username, std::move(roles));
        }
        throw std::runtime_error("Failed to get user: " + id);
    }

    void mongo_db::set_user_username(coco_user &usr, const std::string &username)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("username", username))));
        if (auto result = users_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{usr.get_id()})), doc.view()); !result)
            throw std::runtime_error("Failed to update user username: " + username);
    }

    void mongo_db::set_user_password(coco_user &usr, const std::string &password)
    {
        auto [salt, hash] = encode_password(password);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("salt", salt), bsoncxx::builder::basic::kvp("password", hash))));
        if (auto result = users_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{usr.get_id()})), doc.view()); !result)
            throw std::runtime_error("Failed to update user password");
    }

    void mongo_db::set_user_roles(coco_user &usr, std::set<int> &&roles)
    {
        bsoncxx::builder::basic::document doc;
        auto roles_array = bsoncxx::builder::basic::array{};
        for (const auto &r : roles)
            roles_array.append(r);
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("roles", roles_array))));
        if (auto result = users_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{usr.get_id()})), doc.view()); !result)
            throw std::runtime_error("Failed to update user roles");
    }

    void mongo_db::delete_user(const coco_user &usr)
    {
        if (auto result = users_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{usr.get_id()}))); !result)
            throw std::runtime_error("Failed to delete user: " + usr.get_username());
    }
#endif

    type &mongo_db::create_type(coco_core &cc, const std::string &name, const std::string &description, json::json &&props, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("description", description));
        doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        if (!parents.empty())
        {
            auto parents_array = bsoncxx::builder::basic::array{};
            for (const auto &p : parents)
                parents_array.append(bsoncxx::oid{p.get().get_id()});
            doc.append(bsoncxx::builder::basic::kvp("parents", parents_array));
        }
        if (!static_properties.empty())
        {
            auto static_props = bsoncxx::builder::basic::document{};
            for (const auto &p : static_properties)
                static_props.append(bsoncxx::builder::basic::kvp(p->get_name(), bsoncxx::from_json(to_json(*p).dump())));
            doc.append(bsoncxx::builder::basic::kvp("static_properties", static_props));
        }
        if (!dynamic_properties.empty())
        {
            auto dynamic_props = bsoncxx::builder::basic::document{};
            for (const auto &p : dynamic_properties)
                dynamic_props.append(bsoncxx::builder::basic::kvp(p->get_name(), bsoncxx::from_json(to_json(*p).dump())));
            doc.append(bsoncxx::builder::basic::kvp("dynamic_properties", dynamic_props));
        }
        auto result = types_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_type(cc, result->inserted_id().get_oid().value.to_string(), name, description, std::move(props), std::move(parents), std::move(static_properties), std::move(dynamic_properties));
        throw std::invalid_argument("Failed to insert type: " + name);
    }

    void mongo_db::set_type_name(type &tp, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update type name: " + name);
        coco_db::set_type_name(tp, name);
    }
    void mongo_db::set_type_description(type &tp, const std::string &description)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("description", description))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update type description: " + description);
        coco_db::set_type_description(tp, description);
    }
    void mongo_db::set_type_properties(type &tp, json::json &&props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update type properties: " + props.dump());
        coco_db::set_type_properties(tp, std::move(props));
    }
    void mongo_db::add_static_property(type &tp, std::unique_ptr<property> &&prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_properties." + prop->get_name(), bsoncxx::from_json(to_json(*prop).dump())))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to add static property: " + prop->get_name());
        coco_db::add_static_property(tp, std::move(prop));
    }
    void mongo_db::remove_static_property(type &tp, const property &prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$unset", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("static_properties." + prop.get_name(), ""))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to remove static property: " + prop.get_name());
        coco_db::remove_static_property(tp, prop);
    }
    void mongo_db::add_dynamic_property(type &tp, std::unique_ptr<property> &&prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_properties." + prop->get_name(), bsoncxx::from_json(to_json(*prop).dump())))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to add dynamic property: " + prop->get_name());
        coco_db::add_dynamic_property(tp, std::move(prop));
    }
    void mongo_db::remove_dynamic_property(type &tp, const property &prop)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$unset", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("dynamic_properties." + prop.get_name(), ""))));
        auto result = types_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to remove dynamic property: " + prop.get_name());
        coco_db::remove_dynamic_property(tp, prop);
    }

    void mongo_db::delete_type(const type &tp)
    {
        // Remove all items of this type
        for (const auto &it : get_items())
            if (&it.get().get_type() == &tp)
                delete_item(it);
        auto result = types_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{tp.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete type: " + tp.get_name());
        coco_db::delete_type(tp);
    }

    item &mongo_db::create_item(coco_core &cc, const type &tp, json::json &&props, const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("type_id", bsoncxx::oid{tp.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())));
        bsoncxx::builder::basic::document data_doc;
        data_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(val.dump())));
        data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        doc.append(bsoncxx::builder::basic::kvp("value", data_doc));
        auto result = items_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_item(cc, result->inserted_id().get_oid().value.to_string(), tp, std::move(props), val, timestamp);
        throw std::invalid_argument("Failed to insert item: " + tp.get_name());
    }

    void mongo_db::set_item_properties(item &it, json::json &&props)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("properties", bsoncxx::from_json(props.dump())))));
        auto result = items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update item properties: " + props.dump());
        coco_db::set_item_properties(it, std::move(props));
    }

    void mongo_db::set_item_value(item &it, const json::json &value, const std::chrono::system_clock::time_point &timestamp)
    {
        coco_db::set_item_value(it, value, timestamp);
        bsoncxx::builder::basic::document doc;
        bsoncxx::builder::basic::document data_doc;
        data_doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(it.get_value().dump())));
        data_doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("value", data_doc))));
        auto result = items_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update item value: " + it.get_value().dump());
    }

    void mongo_db::delete_item(const item &it)
    {
        auto result = item_data_collection.delete_many(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete item data for item: " + it.get_id());
        result = items_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{it.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete item: " + it.get_id());
        coco_db::delete_item(it);
    }

    [[nodiscard]] json::json mongo_db::get_data(const item &it, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        bsoncxx::builder::basic::document query;
        query.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()}));
        query.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("$gte", bsoncxx::types::b_date{from}), bsoncxx::builder::basic::kvp("$lte", bsoncxx::types::b_date{to}))));
        json::json data = json::json_type::array;
        for (const auto &doc : item_data_collection.find(query.view()))
            data.push_back(json::json{{"data", json::load(bsoncxx::to_json(doc["data"].get_document().view()))}, {"timestamp", doc["timestamp"].get_date().to_int64()}});
        return data;
    }

    void mongo_db::add_data(item &it, const json::json &data, const std::chrono::system_clock::time_point &timestamp)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("item_id", bsoncxx::oid{it.get_id()}));
        doc.append(bsoncxx::builder::basic::kvp("data", bsoncxx::from_json(data.dump())));
        doc.append(bsoncxx::builder::basic::kvp("timestamp", bsoncxx::types::b_date{timestamp}));
        auto result = item_data_collection.insert_one(doc.view());
        if (!result)
            throw std::invalid_argument("Failed to insert data for item: " + it.get_id());
    }

    rule &mongo_db::create_reactive_rule(coco_core &cc, const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = reactive_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_reactive_rule(cc, result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert reactive rule: " + name);
    }

    void mongo_db::set_reactive_rule_name(rule &rl, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        auto result = reactive_rules_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update reactive rule name: " + name);
        coco_db::set_reactive_rule_name(rl, name);
    }

    void mongo_db::set_reactive_rule_content(rule &rl, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("content", content))));
        auto result = reactive_rules_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update reactive rule content: " + content);
        coco_db::set_reactive_rule_content(rl, content);
    }

    void mongo_db::delete_reactive_rule(const rule &rl)
    {
        auto result = reactive_rules_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete reactive rule: " + rl.get_name());
        coco_db::delete_reactive_rule(rl);
    }

    rule &mongo_db::create_deliberative_rule(coco_core &cc, const std::string &name, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", name));
        doc.append(bsoncxx::builder::basic::kvp("content", content));
        auto result = deliberative_rules_collection.insert_one(doc.view());
        if (result)
            return coco_db::create_deliberative_rule(cc, result->inserted_id().get_oid().value.to_string(), name, content);
        throw std::invalid_argument("Failed to insert deliberative rule: " + name);
    }

    void mongo_db::set_deliberative_rule_name(rule &rl, const std::string &name)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("name", name))));
        auto result = deliberative_rules_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update deliberative rule name: " + name);
        coco_db::set_deliberative_rule_name(rl, name);
    }

    void mongo_db::set_deliberative_rule_content(rule &rl, const std::string &content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("$set", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("content", content))));
        auto result = deliberative_rules_collection.update_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})), doc.view());
        if (!result)
            throw std::invalid_argument("Failed to update deliberative rule content: " + content);
        coco_db::set_deliberative_rule_content(rl, content);
    }

    void mongo_db::delete_deliberative_rule(const rule &rl)
    {
        auto result = deliberative_rules_collection.delete_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", bsoncxx::oid{rl.get_id()})));
        if (!result)
            throw std::invalid_argument("Failed to delete deliberative rule: " + rl.get_name());
        coco_db::delete_deliberative_rule(rl);
    }
} // namespace coco
