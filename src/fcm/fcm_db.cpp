#include "fcm_db.hpp"
#include <cassert>

namespace coco
{
    fcm_db::fcm_db(mongo_db &db) noexcept : mongo_module(db), fcm_collection(get_db()["fcm_tokens"]) { assert(fcm_collection); }

    void fcm_db::add_token(std::string_view id, std::string_view token)
    {
        bsoncxx::builder::basic::document filter, update;
        filter.append(bsoncxx::builder::basic::kvp("_id", id.data()));
        update.append(bsoncxx::builder::basic::kvp("$addToSet", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", token.data()))));
        fcm_collection.update_one(filter.view(), update.view(), mongocxx::options::update{}.upsert(true));
    }

    void fcm_db::remove_token(std::string_view id, std::string_view token)
    {
        bsoncxx::builder::basic::document filter, update;
        filter.append(bsoncxx::builder::basic::kvp("_id", id.data()));
        update.append(bsoncxx::builder::basic::kvp("$pull", bsoncxx::builder::basic::make_document(bsoncxx::builder::basic::kvp("tokens", token.data()))));
        fcm_collection.update_one(filter.view(), update.view());
    }

    std::vector<std::string> fcm_db::get_tokens(std::string_view id) noexcept
    {
        std::vector<std::string> tokens;
        bsoncxx::builder::basic::document filter;
        filter.append(bsoncxx::builder::basic::kvp("_id", id.data()));
        auto doc = fcm_collection.find_one(filter.view());
        if (doc && doc->view().find("tokens") != doc->view().end())
        {
            auto arr = (*doc)["tokens"].get_array().value;
            for (const auto &elem : arr)
                tokens.push_back(elem.get_string().value.data());
        }
        return tokens;
    }
} // namespace coco