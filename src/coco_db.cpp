#include "coco_db.hpp"
#include "logging.hpp"

namespace coco
{
    coco_db::coco_db(const json::json &config) : config(config) {}

    void coco_db::init(coco_core &)
    {
        LOG_DEBUG("Initializing database");
        types_by_name.clear();
        types.clear();
        items.clear();
        reactive_rules_by_name.clear();
        reactive_rules.clear();
        deliberative_rules_by_name.clear();
        deliberative_rules.clear();
    }
} // namespace coco