#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#include "coco.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#include <thread>
#endif

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
    const std::filesystem::path config_root = PROJECT_ROOT;
    coco::set_types(cc, config_root / "types");
    cc.load_rules();
    coco::set_rules(cc, config_root / "rules");
    if (cc.get_items().empty())
        coco::set_items(cc, config_root / "items");
    LOG_INFO("Configuration loaded successfully");

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.start(); });
#endif

    LOG_INFO("Testing item manipulation");
    auto &itm = cc.get_items().front().get();
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

#ifdef INTERACTIVE_TEST
    std::string user_input;
    std::cin >> user_input;
    if (user_input == "d")
        db.drop();
#else
    db.drop();
#endif

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
