#include "auth_db.hpp"
#include "crypto.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    auth_db::auth_db(mongo_db &db, std::string_view mongodb_users_uri) noexcept : mongo_module(db), users_conn(mongocxx::uri(mongodb_users_uri.data())), users_db(users_conn[static_cast<std::string>(db.get_config()["name"]) + "_users"]), users_collection(users_db["users"])
    {
        assert(users_conn);
        for ([[maybe_unused]] const auto &c : users_conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));

        assert(users_db);
        assert(users_collection);

        if (users_collection.list_indexes().begin() == users_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for users collection");
            users_collection.create_index(bsoncxx::builder::stream::document{} << "username" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    db_user auth_db::get_user(std::string_view id)
    {
        auto doc = users_collection.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("_id", id.data())));
        if (!doc)
            throw std::invalid_argument("User not found: " + std::string(id));
        if (doc->view().find("personal_data") == doc->view().end())
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::json{}};
        else
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::load(bsoncxx::to_json(doc->view()["personal_data"].get_document().view()))};
    }

    db_user auth_db::get_user(std::string_view username, std::string_view password)
    {
        auto doc = users_collection.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("username", username.data())));
        if (!doc)
            throw std::invalid_argument("User not found: " + std::string(username));
        if (auto salt = std::string(doc->view()["salt"].get_string().value); utils::encode_password(password, salt) != std::string(doc->view()["password"].get_string().value))
            throw std::invalid_argument("Invalid password for user: " + std::string(username));
        if (doc->view().find("personal_data") == doc->view().end())
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::json{}};
        else
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::load(bsoncxx::to_json(doc->view()["personal_data"].get_document().view()))};
    }

    std::vector<db_user> auth_db::get_users() noexcept
    {
        std::vector<db_user> users;
        auto cursor = users_collection.find({});
        for (const auto &doc : cursor)
        {
            db_user user;
            user.id = std::string(doc["_id"].get_string().value);
            user.username = std::string(doc["username"].get_string().value);
            if (doc.find("personal_data") != doc.end())
                user.personal_data = json::load(bsoncxx::to_json(doc["personal_data"].get_document().view()));
            users.push_back(std::move(user));
        }
        return users;
    }

    void auth_db::create_user(std::string_view id, std::string_view username, std::string_view password, json::json &&personal_data)
    {
        auto [salt, pass] = utils::encode_password(password);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("_id", id.data()));
        doc.append(bsoncxx::builder::basic::kvp("username", username.data()));
        doc.append(bsoncxx::builder::basic::kvp("password", pass.data()));
        doc.append(bsoncxx::builder::basic::kvp("salt", salt.data()));
        if (!personal_data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("personal_data", bsoncxx::from_json(personal_data.dump())));
        if (!users_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert user: " + std::string(username));
    }

    void auth_db::drop() noexcept
    {
        LOG_WARN("Dropping users database..");
        users_db.drop();
    }
} // namespace coco
