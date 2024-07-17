#include "coco_item.hpp"
#include "coco_core.hpp"

namespace coco
{
    item::item(coco_core &cc, const std::string &id, const type &tp, const std::string &name, const json::json &props) noexcept : cc(cc), id(id), tp(tp), name(name), properties(props)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(cc.env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.c_str());
        FBPutSlotString(item_fact_builder, "name", name.c_str());
        item_fact = FBAssert(item_fact_builder);
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(cc.env, "is_instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "item_id", id.c_str());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type_id", tp.get_id().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        FBDispose(is_instance_of_fact_builder);

        for (const auto &[p_name, p] : tp.get_static_properties())
        {
            FactBuilder *property_fact_builder = CreateFactBuilder(cc.env, (tp.get_name() + "_has_" + p_name).c_str());
            FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
            p->set_value(property_fact_builder, get_properties()[p_name]);
            Fact *property_fact = FBAssert(property_fact_builder);
            FBDispose(property_fact_builder);
            property_facts.emplace(p_name, property_fact);
        }
    }
    item::~item() noexcept
    {
        for (auto &p : property_facts)
            Retract(p.second);
        Retract(is_instance_of);
        Retract(item_fact);
    }
} // namespace coco