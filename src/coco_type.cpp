#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    type::type(coco &cc, std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data)), static_properties(std::move(static_props)), dynamic_properties(std::move(dynamic_props))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotString(type_fact_builder, "name", name.data());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        LOG_TRACE(cc.to_string(type_fact));
        FBDispose(type_fact_builder);
        for (auto &[name, prop] : static_properties.as_object())
            cc.get_property_type(static_cast<std::string>(prop["type"])).make_static_property(*this, name, prop);
        for (auto &[name, prop] : dynamic_properties.as_object())
            cc.get_property_type(static_cast<std::string>(prop["type"])).make_dynamic_property(*this, name, prop);
    }
    type::~type()
    {
        for (auto &p : parent_facts)
            Retract(p.second);
        Retract(type_fact);
        for (auto &[p_name, _] : static_properties.as_object())
            Undeftemplate(FindDeftemplate(cc.env, (name + '_' + p_name).c_str()), cc.env);
        for (auto &[p_name, _] : dynamic_properties.as_object())
            Undeftemplate(FindDeftemplate(cc.env, (name + "_has_" + p_name).c_str()), cc.env);
    }

    void type::set_parents(std::vector<utils::ref_wrapper<const type>> &&parents) noexcept
    {
        // we retract the current parent facts (if any)..
        for (auto &p : parent_facts)
            Retract(p.second);
        parent_facts.clear();

        for (auto &p : parents)
        {
            FactBuilder *is_a_fact_builder = CreateFactBuilder(cc.env, "is_a");
            FBPutSlotSymbol(is_a_fact_builder, "type", name.c_str());
            FBPutSlotSymbol(is_a_fact_builder, "parent", p->name.c_str());
            auto parent_fact = FBAssert(is_a_fact_builder);
            assert(parent_fact);
            LOG_TRACE(cc.to_string(parent_fact));
            FBDispose(is_a_fact_builder);
            this->parents.emplace(p->name, p);
            parent_facts.emplace(p->name, parent_fact);
        }
    }

    item &type::new_instance(std::string_view id, json::json &&props, json::json &&val, const std::chrono::system_clock::time_point &timestamp) noexcept
    {
        auto itm_ptr = utils::make_u_ptr<item>(*this, id, std::move(props), std::move(val), timestamp);
        auto &itm = *itm_ptr;
        instances.emplace(id.data(), std::move(itm_ptr));
        return itm;
    }
} // namespace coco
