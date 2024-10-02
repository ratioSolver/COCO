#include "coco_db.hpp"
#include "coco_core.hpp"
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

    void coco_db::set_type_static_properties(type &tp, std::vector<std::unique_ptr<property>> &&props)
    {
        for (auto &itm : tp.get_core().get_items_by_type(tp))
            itm.get().retract_properties();
        tp.set_static_properties(std::move(props));
    }

    void coco_db::set_type_dynamic_properties(type &tp, std::vector<std::unique_ptr<property>> &&props)
    {
        for (auto &itm : tp.get_core().get_items_by_type(tp))
            itm.get().retract_value();
        tp.set_dynamic_properties(std::move(props));
    }
} // namespace coco