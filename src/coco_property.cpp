#include "coco_core.hpp"
#include <stdexcept>
#include <algorithm>

namespace coco
{
    property::property(const std::string &name, const std::string &description) noexcept : name(name), description(description) {}
    json::json property::to_json() const noexcept { return json::json{{"name", name}, {"description", description}}; }

    integer_property::integer_property(const std::string &name, const std::string &description, std::optional<long> default_value, long min, long max) noexcept : property(name, description), default_value(default_value), min(min), max(max) {}
    bool integer_property::validate(const json::json &j, const json::json &) const noexcept
    {
        if (j.get_type() != json::json_type::number)
            return false;
        long value = j;
        return value >= min && value <= max;
    }
    json::json integer_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        if (default_value.has_value())
            j["default"] = default_value.value();
        if (min != std::numeric_limits<long>::min())
            j["min"] = min;
        if (max != std::numeric_limits<long>::max())
            j["max"] = max;
        j["type"] = "integer";
        return j;
    }
    std::string integer_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type INTEGER)";
        if (default_value.has_value())
            deftemplate += " (default " + std::to_string(default_value.value()) + ")";
        if (min != std::numeric_limits<long>::min() || max != std::numeric_limits<long>::max())
        {
            deftemplate += " (range ";
            if (min != std::numeric_limits<long>::min())
                deftemplate += std::to_string(min);
            else
                deftemplate += "?VARIABLE";
            deftemplate += " ";
            if (max != std::numeric_limits<long>::max())
                deftemplate += std::to_string(max);
            else
                deftemplate += "?VARIABLE";
            deftemplate += ")";
        }
        deftemplate += "))";
        return deftemplate;
    }
    void integer_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotInteger(property_fact_builder, get_name().c_str(), static_cast<long>(value)); }

    float_property::float_property(const std::string &name, const std::string &description, std::optional<double> default_value, double min, double max) noexcept : property(name, description), default_value(default_value), min(min), max(max) {}
    bool float_property::validate(const json::json &j, const json::json &) const noexcept
    {
        if (j.get_type() != json::json_type::number)
            return false;
        double value = j;
        return value >= min && value <= max;
    }
    json::json float_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        if (default_value.has_value())
            j["default"] = default_value.value();
        if (min != -std::numeric_limits<double>::max())
            j["min"] = min;
        if (max != std::numeric_limits<double>::max())
            j["max"] = max;
        j["type"] = "float";
        return j;
    }
    std::string float_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type FLOAT)";
        if (default_value.has_value())
            deftemplate += " (default " + std::to_string(default_value.value()) + ")";
        if (min != -std::numeric_limits<double>::max() || max != std::numeric_limits<double>::max())
        {
            deftemplate += " (range ";
            if (min != -std::numeric_limits<double>::max())
                deftemplate += std::to_string(min);
            else
                deftemplate += "?VARIABLE";
            deftemplate += " ";
            if (max != std::numeric_limits<double>::max())
                deftemplate += std::to_string(max);
            else
                deftemplate += "?VARIABLE";
            deftemplate += ")";
        }
        deftemplate += "))";
        return deftemplate;
    }
    void float_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotFloat(property_fact_builder, get_name().c_str(), static_cast<double>(value)); }

    string_property::string_property(const std::string &name, const std::string &description, std::optional<std::string> default_value) noexcept : property(name, description), default_value(default_value) {}
    bool string_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json string_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "string";
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string string_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type STRING)";
        if (default_value.has_value())
            deftemplate += " (default \"" + default_value.value() + "\")";
        deftemplate += ")";
        return deftemplate;
    }
    void string_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    symbol_property::symbol_property(const std::string &name, const std::string &description, std::optional<std::string> default_value) noexcept : property(name, description), default_value(default_value) {}
    bool symbol_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json symbol_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "symbol";
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string symbol_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type SYMBOL))";
        if (default_value.has_value())
            deftemplate += " (default " + default_value.value() + ")";
        deftemplate += ")";
        return deftemplate;
    }
    void symbol_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    item_property::item_property(const std::string &name, const std::string &description, const type &tp, std::optional<std::string> default_value) noexcept : property(name, description), tp(tp), default_value(default_value) {}
    bool item_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json item_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "item";
        j["item_type"] = tp.get_id();
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string item_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type SYMBOL))";
        if (default_value.has_value())
            deftemplate += " (default " + default_value.value() + ")";
        deftemplate += ")";
        return deftemplate;
    }
    void item_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    json_property::json_property(const std::string &name, const std::string &description, json::json &&schema, std::optional<json::json> default_value) noexcept : property(name, description), schema(std::move(schema)), default_value(default_value) {}
    bool json_property::validate(const json::json &j, const json::json &schema_refs) const noexcept { return j.get_type() == json::json_type::object && json::validate(j, schema, schema_refs); }
    json::json json_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "json";
        j["schema"] = schema;
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string json_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name() + " (timestamp (type INTEGER))";
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type STRING)";
        if (default_value.has_value())
            deftemplate += " (default \"" + default_value.value().dump() + "\")";
        deftemplate += ")";
        return deftemplate;
    }
    void json_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, get_name().c_str(), value.dump().c_str()); }

    std::unique_ptr<property> make_property(coco_core &cc, const json::json &j)
    {
        if (j["type"] == "integer")
        {
            std::optional<long> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            long min = std::numeric_limits<long>::min();
            if (j.contains("min"))
                min = j["min"];
            long max = std::numeric_limits<long>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<integer_property>(j["name"], j["description"], default_value, min, max);
        }
        if (j["type"] == "float")
        {
            std::optional<double> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            double min = -std::numeric_limits<double>::max();
            if (j.contains("min"))
                min = j["min"];
            double max = std::numeric_limits<double>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<float_property>(j["name"], j["description"], default_value, min, max);
        }
        if (j["type"] == "string")
        {
            std::optional<std::string> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<string_property>(j["name"], j["description"], default_value);
        }
        if (j["type"] == "symbol")
        {
            std::optional<std::string> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<symbol_property>(j["name"], j["description"]);
        }
        if (j["type"] == "item")
        {
            std::optional<std::string> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<item_property>(j["name"], j["description"], cc.get_type(j["item_type"]), default_value);
        }
        if (j["type"] == "json")
        {
            json::json schema = j["schema"];
            std::optional<json::json> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<json_property>(j["name"], j["description"], std::move(schema), default_value);
        }
        throw std::runtime_error("Unknown property type");
    }
} // namespace coco
