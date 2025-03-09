#include "coco_property.hpp"
#include "coco_type.hpp"
#include "coco.hpp"
#include "logging.hpp"

namespace coco
{
    property_type::property_type(coco &cc, std::string_view name) noexcept : cc(cc), name(name) {}
    Environment *property_type::get_env() { return cc.env; }

    bool_property_type::bool_property_type(coco &cc) noexcept : property_type(cc, bool_kw) {}
    void bool_property_type::make_static_property(type &tp, std::string_view name, const json::json &j) noexcept
    {
        std::string deftemplate = "(deftemplate " + tp.get_name() + '_' + name.data() + " (slot item_id (type SYMBOL)) (slot " + name.data() + " (type SYMBOL)";
        if (j.contains("default"))
            deftemplate += static_cast<bool>(j["default"]) ? " (default TRUE)" : " (default FALSE)";
        deftemplate += ')';
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    void bool_property_type::make_dynamic_property(type &tp, std::string_view name, const json::json &j) noexcept
    {
        std::string deftemplate = "(deftemplate " + tp.get_name() + "_has_" + name.data() + " (slot item_id (type SYMBOL)) (slot timestamp (type INTEGER)) (slot " + name.data() + " (type SYMBOL)";
        if (j.contains("default"))
            deftemplate += static_cast<bool>(j["default"]) ? " (default TRUE)" : " (default FALSE)";
        deftemplate += ')';
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    void bool_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, name.data(), static_cast<bool>(value) ? "TRUE" : "FALSE"); }
} // namespace coco
