#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include <cassert>

namespace coco
{
    type::type(coco &cc, const std::string &name, const json::json &static_props, const json::json &dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotString(type_fact_builder, "name", name.c_str());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        FBDispose(type_fact_builder);
        for (auto &[name, prop] : static_props.as_object())
        {
            auto p = cc.make_property(*this, false, name, prop);
            Build(cc.env, p->to_deftemplate().c_str());
            properties.emplace(name, std::move(p));
        }
        for (auto &[name, prop] : dynamic_props.as_object())
        {
            auto p = cc.make_property(*this, true, name, prop);
            Build(cc.env, p->to_deftemplate().c_str());
            properties.emplace(name, std::move(p));
        }
    }
} // namespace coco
