#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include <cassert>

namespace coco
{
    type::type(coco &cc, const std::string &name, const json::json &props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotString(type_fact_builder, "name", name.c_str());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        FBDispose(type_fact_builder);
        for (auto &[name, prop] : props.as_object())
        {
            cc.make_property(*this, name, prop);
        }
    }
} // namespace coco
