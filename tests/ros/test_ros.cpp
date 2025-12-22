#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#include "coco.hpp"
// #include "coco_ros.hpp"
#include "logging.hpp"
#include "coco_ros/msg/robot.hpp"

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
    // LOG_INFO("Adding CoCo ROS module");
    // cc.add_module<coco::coco_ros>(cc);
    LOG_INFO("Loading configuration");
    const std::filesystem::path config_root = PROJECT_ROOT;
    coco::set_types(cc, config_root / "types");
    cc.load_rules();
    coco::set_rules(cc, config_root / "rules");
    if (cc.get_items().empty())
        coco::set_items(cc, config_root / "items");
    LOG_INFO("Configuration loaded successfully");

    return 0;
}
