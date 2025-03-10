#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    type::type(coco &cc, std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), static_properties(std::move(static_props)), dynamic_properties(std::move(dynamic_props)), data(std::move(data))
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
        Retract(type_fact);
        for (auto &[p_name, _] : static_properties.as_object())
            Undeftemplate(FindDeftemplate(cc.env, (name + '_' + p_name).c_str()), cc.env);
        for (auto &[p_name, _] : dynamic_properties.as_object())
            Undeftemplate(FindDeftemplate(cc.env, (name + "_has_" + p_name).c_str()), cc.env);
    }

    item &type::new_instance(std::string_view id, json::json &&props, json::json &&val, const std::chrono::system_clock::time_point &timestamp) noexcept
    {
        auto itm_ptr = utils::make_u_ptr<item>(*this, id, std::move(props), std::move(val), timestamp);
        auto &itm = *itm_ptr;
        instances.emplace(id.data(), std::move(itm_ptr));
        return itm;
    }
} // namespace coco
