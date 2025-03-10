#include "coco_property.hpp"
#include "coco_type.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    property_type::property_type(coco &cc, std::string_view name) noexcept : cc(cc), name(name) {}

    bool_property_type::bool_property_type(coco &cc) noexcept : property_type(cc, bool_kw) {}
    utils::u_ptr<property> bool_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<bool> default_value;
        if (j.contains("default"))
            default_value = static_cast<bool>(j["default"]);
        return utils::make_u_ptr<bool_property>(*this, tp, dynamic, name, default_value);
    }
    void bool_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, name.data(), static_cast<bool>(value) ? "TRUE" : "FALSE"); }

    property::property(const property_type &pt, const type &tp, bool dynamic, std::string_view name) noexcept : pt(pt), tp(tp), dynamic(dynamic), name(name) {}
    property::~property()
    {
        auto dt = FindDeftemplate(pt.get_coco().env, get_deftemplate_name().c_str());
        assert(dt);
        Undeftemplate(dt, pt.get_coco().env);
    }

    std::string property::get_deftemplate_name()
    {
        std::string deftemplate = tp.get_name();
        if (dynamic)
            deftemplate += "_has_";
        else
            deftemplate += '_';
        deftemplate += name.data();
        return deftemplate;
    }
    Environment *property::get_env() const noexcept { return pt.get_coco().env; }
    const json::json &property::get_schemas() const noexcept { return pt.get_coco().schemas; }

    bool_property::bool_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<bool> default_value) noexcept : property(pt, tp, dynamic, name), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type SYMBOL)";
        if (default_value.has_value())
            deftemplate += default_value.value() ? " (default TRUE)" : " (default FALSE)";
        deftemplate += "))";
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool bool_property::validate(const json::json &j) const noexcept { return j.get_type() == json::json_type::boolean; }
} // namespace coco
