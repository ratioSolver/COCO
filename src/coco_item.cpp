#include "coco_item.hpp"
#include "coco_core.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    item::item(coco_core &cc, const std::string &id, const type &tp, json::json &&props, const json::json &val, const std::chrono::system_clock::time_point &timestamp) noexcept : cc(cc), id(id), tp(tp)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(cc.env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.c_str());
        item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(cc.env, "is_instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "item_id", id.c_str());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type_id", tp.get_id().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        assert(is_instance_of);
        FBDispose(is_instance_of_fact_builder);

        set_properties(std::move(props));
        set_value(val, timestamp);
    }
    item::~item() noexcept
    {
        for (auto &p : property_facts)
            Retract(p.second);
        Retract(is_instance_of);
        Retract(item_fact);
    }

    void item::set_properties(json::json &&props)
    {
        for (auto &p : property_facts)
            Retract(p.second);
        property_facts.clear();

        // Set the properties
        std::queue<const type *> q;
        q.push(&tp);
        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_static_properties())
            {
                FactBuilder *property_fact_builder = CreateFactBuilder(cc.env, p->to_deftemplate_name(*t, false).c_str());
                FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
                p->set_value(property_fact_builder, props[p_name]);
                auto property_fact = FBAssert(property_fact_builder);
                assert(property_fact);
                FBDispose(property_fact_builder);
                properties[p_name] = props[p_name];
                property_facts[p_name] = property_fact;
            }

            for (const auto &tp : t->get_parents())
                q.push(&tp.second.get());
        }
    }

    void item::set_value(const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        std::queue<const type *> q;
        q.push(&tp);
        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_dynamic_properties())
                if (val.contains(p_name))
                {
                    if (!p->validate(val[p_name], cc.schemas))
                        LOG_WARN("Data " + p_name + " for item " + id + " is invalid");
                    if (value_facts.find(p_name) != value_facts.end())
                        Retract(value_facts[p_name]); // Retract the old value
                    FactBuilder *value_fact_builder = CreateFactBuilder(cc.env, p->to_deftemplate_name(*t).c_str());
                    FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                    p->set_value(value_fact_builder, val[p_name]);
                    FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
                    auto value_fact = FBAssert(value_fact_builder);
                    assert(value_fact);
                    FBDispose(value_fact_builder);
                    value[p_name] = val[p_name];
                    value_facts[p_name] = value_fact;
                }
                else
                    LOG_WARN("Data for item " + id + " do not contain " + p_name);

            for (const auto &tp : t->get_parents())
                q.push(&tp.second.get());
        }

        this->timestamp = timestamp;
    }
} // namespace coco