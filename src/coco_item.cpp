#include "coco_item.hpp"
#include "coco_core.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    item::item(const std::string &id, const type &tp, json::json &&props, const json::json &val, const std::chrono::system_clock::time_point &timestamp) noexcept : id(id), tp(tp)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(tp.get_core().env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.c_str());
        item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(tp.get_core().env, "is_instance_of");
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
        for (auto &v : value_facts)
            Retract(v.second);
        for (auto &p : property_facts)
            Retract(p.second);
        Retract(is_instance_of);
        Retract(item_fact);
    }

    void item::set_properties(json::json &&props)
    {
        std::map<std::string, std::reference_wrapper<const property>> static_properties;
        std::queue<const type *> q;
        q.push(&tp);
        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_static_properties())
                static_properties.emplace(p_name, *p);

            for (const auto &tp : t->get_parents())
                q.push(&tp.second.get());
        }

        for (const auto &[p_name, val] : props.as_object())
            if (static_properties.find(p_name) != static_properties.end())
            {
                if (auto f = property_facts.find(p_name); f != property_facts.end())
                { // we retract the old property
                    Retract(f->second);
                    property_facts.erase(f);
                }

                if (val.get_type() == json::json_type::null)
                    this->properties.erase(p_name); // we remove the property
                else if (static_properties.at(p_name).get().validate(val, tp.get_core().schemas))
                {
                    FactBuilder *property_fact_builder = CreateFactBuilder(tp.get_core().env, static_properties.at(p_name).get().to_deftemplate_name(false).c_str());
                    FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
                    static_properties.at(p_name).get().set_value(property_fact_builder, val);
                    auto property_fact = FBAssert(property_fact_builder);
                    assert(property_fact);
                    FBDispose(property_fact_builder);
                    this->properties[p_name] = val;
                    property_facts.emplace(p_name, property_fact);
                }
                else
                    LOG_WARN("Property " + p_name + " for item " + id + " is invalid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have static property " + p_name);
    }

    void item::retract_properties()
    {
        for (auto &p : property_facts)
            Retract(p.second);
    }

    void item::set_value(const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        std::map<std::string, std::reference_wrapper<const property>> dynamic_properties;
        std::queue<const type *> q;
        q.push(&tp);
        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->get_dynamic_properties())
                dynamic_properties.emplace(p_name, *p);

            for (const auto &tp : t->get_parents())
                q.push(&tp.second.get());
        }

        for (const auto &[p_name, val] : val.as_object())
            if (dynamic_properties.find(p_name) != dynamic_properties.end())
            {
                if (auto f = value_facts.find(p_name); f != value_facts.end())
                { // we retract the old property
                    Retract(f->second);
                    value_facts.erase(f);
                }

                if (val.get_type() == json::json_type::null)
                    this->value.erase(p_name); // we remove the property
                else if (dynamic_properties.at(p_name).get().validate(val, tp.get_core().schemas))
                {
                    FactBuilder *value_fact_builder = CreateFactBuilder(tp.get_core().env, dynamic_properties.at(p_name).get().to_deftemplate_name().c_str());
                    FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                    dynamic_properties.at(p_name).get().set_value(value_fact_builder, val[p_name]);
                    FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count());
                    auto value_fact = FBAssert(value_fact_builder);
                    assert(value_fact);
                    FBDispose(value_fact_builder);
                    value[p_name] = val[p_name];
                    value_facts.emplace(p_name, value_fact);
                }
                else
                    LOG_WARN("Data " + p_name + " for item " + id + " is invalid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have dynamic property " + p_name);

        this->timestamp = timestamp;
    }

    void item::retract_value()
    {
        for (auto &v : value_facts)
            Retract(v.second);
    }
} // namespace coco