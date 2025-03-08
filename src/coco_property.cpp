#include "coco_property.hpp"
#include "coco_type.hpp"

namespace coco
{
    property::property(const type &tp, const std::string &name, bool dynamic) noexcept : tp(tp), name(name), dynamic(dynamic) {}
    std::string property::to_deftemplate() const noexcept
    {
        std::string deftemplate = "(deftemplate ";
        if (dynamic)
            deftemplate += tp.get_name() + "_has_" + name;
        else
            deftemplate += tp.get_name() + '_' + name;
        deftemplate += " (slot item_id (type SYMBOL))";
        deftemplate += '_' + get_deftemplate_slot();
        deftemplate += ')';
        return deftemplate;
    }

    boolean_property::boolean_property(const type &tp, const std::string &name, const json::json &j) noexcept : property(tp, name, j.contains("description") ? j["description"] : "")
    {
        if (j.contains("default"))
            default_value = j["default"];
    }
    json::json boolean_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = bool_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    void boolean_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<bool>(value) ? "TRUE" : "FALSE"); }
    std::string boolean_property::get_deftemplate_slot() const noexcept
    {
        std::string slot = "(slot " + get_name() + " (type SYMBOL)";
        if (default_value.has_value())
            slot += default_value.value() ? " (default TRUE)" : " (default FALSE)";
        slot += ")";
        return slot;
    }
} // namespace coco
