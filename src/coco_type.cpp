#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef BUILD_LISTENERS
#define NEW_ITEM(itm) cc.new_item(itm)
#else
#define NEW_ITEM(itm)
#endif

namespace coco
{
    type::type(coco &cc, std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotSymbol(type_fact_builder, "name", name.data());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        LOG_TRACE(cc.to_string(type_fact));
        FBDispose(type_fact_builder);
        set_parents(std::move(parents));
        for (auto &[name, prop] : static_props.as_object())
            static_properties.emplace(name, cc.get_property_type(static_cast<std::string>(prop["type"])).new_instance(*this, false, name, prop));
        for (auto &[name, prop] : dynamic_props.as_object())
            dynamic_properties.emplace(name, cc.get_property_type(static_cast<std::string>(prop["type"])).new_instance(*this, true, name, prop));
    }
    type::~type()
    {
        instances.clear();
        for (auto &p : parent_facts)
            Retract(p.second);
        Retract(type_fact);
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

    item &type::make_item(std::string_view id, json::json &&props, json::json &&val, const std::chrono::system_clock::time_point &timestamp) noexcept
    {
        auto itm_ptr = utils::make_u_ptr<item>(*this, id, std::move(props), std::move(val), timestamp);
        auto &itm = *itm_ptr;
        instances.emplace(id.data(), std::move(itm_ptr));
        NEW_ITEM(itm);
        return itm;
    }
} // namespace coco
