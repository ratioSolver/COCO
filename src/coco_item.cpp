#include "coco_item.hpp"
#include "coco_type.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

namespace coco
{
    item::item(const type &tp, std::string_view id, json::json &&props, json::json &&val, const std::chrono::system_clock::time_point &timestamp) noexcept : tp(tp), id(id), properties(props), value(val), timestamp(timestamp)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(tp.get_coco().env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.data());
        item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(tp.get_coco().env, "is_instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "item_id", id.data());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type", tp.get_name().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        assert(is_instance_of);
        FBDispose(is_instance_of_fact_builder);

        set_properties(std::move(props));
        set_value(val, timestamp);
    }
    item::~item() noexcept {}

    void item::set_properties(json::json &&props)
    {
        std::queue<const type *> q;
        q.push(&tp);

        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();
            for (const auto &[p_name, val] : props.as_object())
                if (auto prop = t->get_static_properties().as_object().find(p_name); prop != t->get_static_properties().as_object().end())
                {
                    if (auto f = properties_facts.find(p_name); f != properties_facts.end())
                    { // we retract the old property
                        Retract(f->second);
                        properties_facts.erase(f);
                    }

                    if (val.get_type() == json::json_type::null)
                        this->properties.erase(p_name); // we remove the property
                }
        }
    }

    void item::set_value(const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
    }
} // namespace coco
