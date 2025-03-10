#include "coco_item.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
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
        LOG_TRACE(tp.get_coco().to_string(item_fact));
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(tp.get_coco().env, "instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "id", id.data());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type", tp.get_name().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        assert(is_instance_of);
        LOG_TRACE(tp.get_coco().to_string(is_instance_of));
        FBDispose(is_instance_of_fact_builder);

        set_properties(std::move(props));
        set_value(val, timestamp);
    }
    item::~item() noexcept
    {
        for (auto &v : value_facts)
            Retract(v.second);
        for (auto &p : properties_facts)
            Retract(p.second);
        Retract(is_instance_of);
        Retract(item_fact);
    }

    void item::set_properties(json::json &&props)
    {
        std::map<std::string, utils::ref_wrapper<const property>> static_properties;
        std::queue<const type *> q;
        q.push(&tp);

        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_static_properties())
                static_properties.emplace(p_name, *p);

            for (const auto &tp : t->get_parents())
                q.push(&*tp.second);
        }

        for (const auto &[p_name, val] : props.as_object())
            if (auto prop = static_properties.find(p_name); prop != static_properties.end())
            {
                if (auto f = properties_facts.find(p_name); f != properties_facts.end())
                { // we retract the old property
                    Retract(f->second);
                    properties_facts.erase(f);
                }

                if (val.get_type() == json::json_type::null)
                    this->properties.erase(p_name); // we remove the property
                else if (prop->second->validate(val))
                {
                    FactBuilder *property_fact_builder = CreateFactBuilder(tp.get_coco().env, prop->second->get_deftemplate_name().c_str());
                    FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
                    prop->second->get_property_type().set_value(property_fact_builder, p_name, val);
                    auto property_fact = FBAssert(property_fact_builder);
                    assert(property_fact);
                    LOG_TRACE(tp.get_coco().to_string(property_fact));
                    FBDispose(property_fact_builder);
                    this->properties[p_name] = val;
                    properties_facts.emplace(p_name, property_fact);
                }
                else
                    LOG_WARN("Property " + p_name + " for item " + id + " is not valid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have static property " + p_name);
    }

    void item::set_value(const json::json &vals, const std::chrono::system_clock::time_point &timestamp)
    {
        std::map<std::string, utils::ref_wrapper<const property>> dynamic_properties;
        std::queue<const type *> q;
        q.push(&tp);

        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_dynamic_properties())
                dynamic_properties.emplace(p_name, *p);

            for (const auto &tp : t->get_parents())
                q.push(&*tp.second);
        }

        for (const auto &[p_name, val] : vals.as_object())
            if (auto prop = dynamic_properties.find(p_name); prop != dynamic_properties.end())
            {
                if (auto f = properties_facts.find(p_name); f != properties_facts.end())
                { // we retract the old property
                    Retract(f->second);
                    properties_facts.erase(f);
                }

                if (val.get_type() == json::json_type::null)
                    this->properties.erase(p_name); // we remove the property
                else if (prop->second->validate(val))
                {
                    FactBuilder *value_fact_builder = CreateFactBuilder(tp.get_coco().env, prop->second->get_deftemplate_name().c_str());
                    FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                    prop->second->get_property_type().set_value(value_fact_builder, p_name, val);
                    FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
                    auto property_fact = FBAssert(value_fact_builder);
                    assert(property_fact);
                    LOG_TRACE(tp.get_coco().to_string(property_fact));
                    FBDispose(value_fact_builder);
                    this->properties[p_name] = val;
                    properties_facts.emplace(p_name, property_fact);
                }
                else
                    LOG_WARN("Data " + p_name + " for item " + id + " is not valid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have dynamic property " + p_name);

        this->timestamp = timestamp;
    }
} // namespace coco
