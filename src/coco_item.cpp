#include "coco_item.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

#ifdef BUILD_LISTENERS
#define NEW_ITEM() tp.get_coco().new_item(*this)
#define UPDATED_ITEM() tp.get_coco().updated_item(*this)
#define NEW_DATA(data, timestamp) tp.get_coco().new_data(*this, data, timestamp)
#else
#define NEW_ITEM()
#define UPDATED_ITEM()
#define NEW_DATA(data, timestamp)
#endif

namespace coco
{
    item::item(type &tp, std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val) noexcept : tp(tp), id(id), properties(props)
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
        if (val.has_value())
            set_value(std::move(val.value()));

        NEW_ITEM();
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
        UPDATED_ITEM();
    }

    void item::set_value(std::pair<json::json, std::chrono::system_clock::time_point> &&val)
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

        if (!value.has_value())
            value = std::make_pair(json::json(), val.second);
        else
            value->second = val.second;

        for (const auto &[p_name, j_val] : val.first.as_object())
            if (auto prop = dynamic_properties.find(p_name); prop != dynamic_properties.end())
            {
                if (auto f = properties_facts.find(p_name); f != properties_facts.end())
                { // we retract the old property
                    Retract(f->second);
                    properties_facts.erase(f);
                }

                if (j_val.get_type() == json::json_type::null)
                    this->properties.erase(p_name); // we remove the property
                else if (prop->second->validate(j_val))
                {
                    FactBuilder *value_fact_builder = CreateFactBuilder(tp.get_coco().env, prop->second->get_deftemplate_name().c_str());
                    FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                    prop->second->get_property_type().set_value(value_fact_builder, p_name, j_val);
                    FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(val.second.time_since_epoch()).count());
                    auto property_fact = FBAssert(value_fact_builder);
                    assert(property_fact);
                    LOG_TRACE(tp.get_coco().to_string(property_fact));
                    FBDispose(value_fact_builder);
                    value->first[p_name] = j_val;
                    properties_facts.emplace(p_name, property_fact);
                }
                else
                    LOG_WARN("Data " + p_name + " for item " + id + " is not valid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have dynamic property " + p_name);
        NEW_DATA(val.first, val.second);
    }

    json::json item::to_json() const noexcept
    {
        json::json j_itm{{"type", tp.get_name().c_str()}};
        if (!properties.as_object().empty())
            j_itm["properties"] = properties;
        if (value.has_value())
            j_itm["value"] = json::json{{"data", value->first}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(value->second.time_since_epoch()).count()}};
        return j_itm;
    }
} // namespace coco
