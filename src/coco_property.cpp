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

    int_property_type::int_property_type(coco &cc) noexcept : property_type(cc, int_kw) {}
    utils::u_ptr<property> int_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<long> default_value;
        if (j.contains("default"))
            default_value = static_cast<long>(j["default"]);
        std::optional<long> min;
        if (j.contains("min"))
            min = static_cast<long>(j["min"]);
        std::optional<long> max;
        if (j.contains("max"))
            max = static_cast<long>(j["max"]);
        return utils::make_u_ptr<int_property>(*this, tp, dynamic, name, default_value, min, max);
    }
    void int_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotInteger(property_fact_builder, name.data(), static_cast<long>(value)); }

    float_property_type::float_property_type(coco &cc) noexcept : property_type(cc, float_kw) {}
    utils::u_ptr<property> float_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<double> default_value;
        if (j.contains("default"))
            default_value = static_cast<double>(j["default"]);
        std::optional<double> min;
        if (j.contains("min"))
            min = static_cast<double>(j["min"]);
        std::optional<double> max;
        if (j.contains("max"))
            max = static_cast<double>(j["max"]);
        return utils::make_u_ptr<float_property>(*this, tp, dynamic, name, default_value, min, max);
    }
    void float_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotFloat(property_fact_builder, name.data(), static_cast<double>(value)); }

    string_property_type::string_property_type(coco &cc) noexcept : property_type(cc, string_kw) {}
    utils::u_ptr<property> string_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<std::string> default_value;
        if (j.contains("default"))
            default_value = static_cast<std::string>(j["default"]);
        return utils::make_u_ptr<string_property>(*this, tp, dynamic, name, default_value);
    }
    void string_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, name.data(), static_cast<std::string>(value).c_str()); }

    property::property(const property_type &pt, const type &tp, bool dynamic, std::string_view name) noexcept : pt(pt), tp(tp), dynamic(dynamic), name(name) {}
    property::~property()
    {
        auto dt = FindDeftemplate(pt.get_coco().env, get_deftemplate_name().c_str());
        assert(dt);
        Undeftemplate(dt, pt.get_coco().env);
    }

    std::string property::get_deftemplate_name() const noexcept
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
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool bool_property::validate(const json::json &j) const noexcept { return j.get_type() == json::json_type::boolean; }

    int_property::int_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<long> default_value, std::optional<long> min, std::optional<long> max) noexcept : property(pt, tp, dynamic, name), default_value(default_value), min(min), max(max)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type INTEGER)";
        if (default_value.has_value())
            deftemplate += " (default " + std::to_string(default_value.value()) + ")";
        if (min.has_value() || max.has_value())
        {
            deftemplate += " (range ";
            deftemplate += min.has_value() ? std::to_string(min.value()) : "?VARIABLE";
            deftemplate += ' ';
            deftemplate += max.has_value() ? std::to_string(max.value()) : "?VARIABLE";
            deftemplate += ')';
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool int_property::validate(const json::json &j) const noexcept
    {
        if (j.get_type() != json::json_type::number)
            return false;
        long value = j;
        if ((min.has_value() && min.value() > value) || (max.has_value() && max.value() < value))
            return false;
        return true;
    }

    float_property::float_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<double> default_value, std::optional<double> min, std::optional<double> max) noexcept : property(pt, tp, dynamic, name), default_value(default_value), min(min), max(max)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type FLOAT)";
        if (default_value.has_value())
            deftemplate += " (default " + std::to_string(default_value.value()) + ")";
        if (min.has_value() || max.has_value())
        {
            deftemplate += " (range ";
            deftemplate += min.has_value() ? std::to_string(min.value()) : "?VARIABLE";
            deftemplate += ' ';
            deftemplate += max.has_value() ? std::to_string(max.value()) : "?VARIABLE";
            deftemplate += ')';
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool float_property::validate(const json::json &j) const noexcept
    {
        if (j.get_type() != json::json_type::number)
            return false;
        long value = j;
        if ((min.has_value() && min.value() > value) || (max.has_value() && max.value() < value))
            return false;
        return true;
    }

    string_property::string_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<std::string> default_value) noexcept : property(pt, tp, dynamic, name), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type STRING)";
        if (default_value.has_value())
            deftemplate += default_value.value();
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool string_property::validate(const json::json &j) const noexcept { return j.get_type() == json::json_type::string; }
} // namespace coco
