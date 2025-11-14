#include "TestConfig_config.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#include "coco_item.hpp"
#include "logging.hpp"

int main()
{
#ifdef BUILD_MONGODB
    mongocxx::instance inst{}; // This should be done only once.
    LOG_INFO("Creating MongoDB instance");
    coco::mongo_db db;
#else
    LOG_INFO("Creating default CoCo database instance");
    coco::coco_db db;
#endif
    LOG_INFO("Creating CoCo instance");
    coco::coco cc(db);
    LOG_INFO("Loading configuration");
    coco::config(cc);
    LOG_INFO("Configuration loaded successfully");

    std::vector<std::reference_wrapper<coco::type>> types = {cc.get_type("person")};
    auto &itm = cc.create_item(std::move(types), {{"name", "John"}, {"age", 30}});
    LOG_DEBUG("Created item " + itm.to_json().dump());

    cc.set_properties(itm, {{"name", "John Doe"}, {"age", 31}, {"is_active", true}});
    LOG_DEBUG("Updated item properties: " + itm.to_json().dump());
    cc.set_properties(itm, {{"is_active", true}});
    LOG_DEBUG("Updated item properties with is_active true: " + itm.to_json().dump());
    cc.set_properties(itm, {{"is_active", false}});
    LOG_DEBUG("Updated item properties with is_active false: " + itm.to_json().dump());
    cc.set_value(itm, {{"is_employed", true}});
    LOG_DEBUG("Set item value: " + itm.to_json().dump());
    cc.set_value(itm, {{"is_employed", false}});
    LOG_DEBUG("Updated item value: " + itm.to_json().dump());

    return 0;
}
