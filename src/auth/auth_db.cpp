#include "auth_db.hpp"
#include "crypto.hpp"
#include "logging.hpp"
#include <bsoncxx/json.hpp>
#include <cassert>

namespace coco
{
    auth_db::auth_db(mongo_db &db, std::string_view mongodb_users_uri) noexcept : mongo_module(db), users_conn(mongocxx::uri(mongodb_users_uri.data())), users_db(users_conn[static_cast<std::string>(db.get_config()["name"]) + "_users"]), users_collection(users_db["users"])
    {
        assert(users_conn);
        for ([[maybe_unused]] const auto &c : users_conn.uri().hosts())
            LOG_DEBUG("Connected to MongoDB server at " + c.name + ":" + std::to_string(c.port));
    }

    db_user auth_db::get_user(std::string_view username, std::string_view password)
    {
        auto doc = users_collection.find_one(bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("username", username.data())));
        if (!doc)
            throw std::invalid_argument("User not found: " + std::string(username));
        if (auto salt = doc->view()["salt"].get_string().value.to_string(); utils::encode_password(password, salt) != doc->view()["password"].get_string().value.to_string())
            throw std::invalid_argument("Invalid password for user: " + std::string(username));
        if (doc->view().find("personal_data") == doc->view().end())
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::json{}};
        else
            return db_user{std::string(doc->view()["_id"].get_string().value), std::string(doc->view()["username"].get_string().value), json::load(bsoncxx::to_json(doc->view()["personal_data"].get_document().view()))};
    }

    void auth_db::create_user(std::string_view itm_id, std::string_view username, std::string_view password, json::json &&personal_data)
    {
        auto [salt, hash] = utils::encode_password(password);
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("_id", itm_id.data()));
        doc.append(bsoncxx::builder::basic::kvp("username", username.data()));
        doc.append(bsoncxx::builder::basic::kvp("password", password.data()));
        doc.append(bsoncxx::builder::basic::kvp("salt", salt.data()));
        if (!personal_data.as_object().empty())
            doc.append(bsoncxx::builder::basic::kvp("personal_data", bsoncxx::from_json(personal_data.dump())));
        if (!users_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert user: " + std::string(username));
    }
} // namespace coco
