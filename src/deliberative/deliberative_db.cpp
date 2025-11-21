#include "deliberative_db.hpp"
#include "logging.hpp"
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>

namespace coco
{
    deliberative_db::deliberative_db(mongo_db &db) noexcept : mongo_module(db)
    {
        auto client = get_client();
        auto database = (*client)[db.get_db_name()];
        auto deliberative_rules_collection = database[deliberative_collection_name];
        assert(deliberative_rules_collection);
        if (deliberative_rules_collection.list_indexes().begin() == deliberative_rules_collection.list_indexes().end())
        {
            LOG_DEBUG("Creating indexes for deliberative rules collection");
            deliberative_rules_collection.create_index(bsoncxx::builder::stream::document{} << "name" << 1 << bsoncxx::builder::stream::finalize, mongocxx::options::index{}.unique(true));
        }
    }

    std::vector<db_rule> deliberative_db::get_deliberative_rules() noexcept
    {
        std::vector<db_rule> rules;
        auto client = get_client();
        auto database = (*client)[db.get_db_name()];
        auto deliberative_rules_collection = database[deliberative_collection_name];
        assert(deliberative_rules_collection);
        for (const auto &doc : deliberative_rules_collection.find({}))
        {
            auto name = doc["name"].get_string().value;
            auto content = doc["content"].get_string().value;
            rules.push_back({std::string(name), std::string(content)});
        }
        return rules;
    }
    void deliberative_db::create_deliberative_rule(std::string_view rule_name, std::string_view rule_content)
    {
        bsoncxx::builder::basic::document doc;
        doc.append(bsoncxx::builder::basic::kvp("name", rule_name.data()));
        doc.append(bsoncxx::builder::basic::kvp("content", rule_content.data()));
        auto client = get_client();
        auto database = (*client)[db.get_db_name()];
        auto deliberative_rules_collection = database[deliberative_collection_name];
        assert(deliberative_rules_collection);
        if (!deliberative_rules_collection.insert_one(doc.view()))
            throw std::invalid_argument("Failed to insert reactive rule: " + std::string(rule_name));
    }
} // namespace coco
