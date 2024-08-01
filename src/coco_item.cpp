#include "coco_item.hpp"
#include "coco_core.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    item::item(coco_core &cc, const std::string &id, const type &tp, const std::string &name, const json::json &props) noexcept : cc(cc), id(id), tp(tp), name(name), properties(props)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(cc.env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.c_str());
        FBPutSlotString(item_fact_builder, "name", name.c_str());
        item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(cc.env, "is_instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "item_id", id.c_str());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type_id", tp.get_id().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        assert(is_instance_of);
        FBDispose(is_instance_of_fact_builder);

        set_properties(props);
    }
    item::~item() noexcept
    {
        for (auto &p : property_facts)
            Retract(p.second);
        Retract(is_instance_of);
        Retract(item_fact);
    }

    void item::set_name(const std::string &name)
    {
        FactModifier *fm = CreateFactModifier(cc.env, item_fact);
        FMPutSlotString(fm, "name", name.c_str());
        item_fact = FMModify(fm);
        FMDispose(fm);
    }

    void item::set_properties(const json::json &props)
    {
        for (auto &p : property_facts)
            Retract(p.second);
        property_facts.clear();
        for (const auto &[p_name, p] : tp.get_static_properties())
        {
            FactBuilder *property_fact_builder = CreateFactBuilder(cc.env, p->to_deftemplate_name(tp, false).c_str());
            FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
            p->set_value(property_fact_builder, properties[p_name]);
            Fact *property_fact = FBAssert(property_fact_builder);
            assert(property_fact);
            FBDispose(property_fact_builder);
            property_facts[p_name] = property_fact;
        }
        properties = props;
    }

    void item::set_value(const json::json &value, const std::chrono::system_clock::time_point &timestamp)
    {
        for (const auto &[p_name, p] : tp.get_dynamic_properties())
            if (value.contains(p_name))
            {
                FactBuilder *value_fact_builder = CreateFactBuilder(cc.env, p->to_deftemplate_name(tp).c_str());
                FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                p->set_value(value_fact_builder, value);
                FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
                [[maybe_unused]] Fact *value_fact = FBAssert(value_fact_builder);
                assert(value_fact);
                FBDispose(value_fact_builder);
            }
    }
} // namespace coco