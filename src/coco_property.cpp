#include "coco_property.hpp"
#include "coco_type.hpp"
#include <stdexcept>
#include <algorithm>

namespace coco
{
    property::property(const std::string &name, const std::string &description) noexcept : name(name), description(description) {}
    json::json property::to_json() const noexcept { return json::json{{"name", name}, {"description", description}}; }

    integer_property::integer_property(const std::string &name, const std::string &description, long min, long max) noexcept : property(name, description), min(min), max(max) {}
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
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name();
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type INTEGER) (default 0)";
        if (min != std::numeric_limits<long>::min() || max != std::numeric_limits<long>::max())
            deftemplate += " (range " + std::to_string(min) + " " + std::to_string(max) + ")";
        deftemplate += "))";
        return deftemplate;
    }
    void integer_property::set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept { FBPutSlotInteger(item_fact_builder, get_name().c_str(), static_cast<long>(value)); }

    float_property::float_property(const std::string &name, const std::string &description, double min, double max) noexcept : property(name, description), min(min), max(max) {}
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
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name();
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type FLOAT) (default 0.0)";
        if (min != -std::numeric_limits<double>::max() || max != std::numeric_limits<double>::max())
            deftemplate += " (range " + std::to_string(min) + " " + std::to_string(max) + ")";
        deftemplate += "))";
        return deftemplate;
    }
    void float_property::set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept { FBPutSlotFloat(item_fact_builder, get_name().c_str(), static_cast<double>(value)); }

    string_property::string_property(const std::string &name, const std::string &description) noexcept : property(name, description) {}
    bool string_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json string_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "string";
        return j;
    }
    std::string string_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name();
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type STRING) (default \"\"))";
        deftemplate += ")";
        return deftemplate;
    }
    void string_property::set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept { FBPutSlotString(item_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    symbol_property::symbol_property(const std::string &name, const std::string &description) noexcept : property(name, description) {}
    bool symbol_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json symbol_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "symbol";
        return j;
    }
    std::string symbol_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name();
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type SYMBOL))";
        deftemplate += ")";
        return deftemplate;
    }
    void symbol_property::set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(item_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    json_property::json_property(const std::string &name, const std::string &description, json::json &&schema) noexcept : property(name, description), schema(std::move(schema)) {}
    bool json_property::validate(const json::json &j, const json::json &schema_refs) const noexcept { return j.get_type() == json::json_type::object && json::validate(j, schema, schema_refs); }
    json::json json_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["schema"] = schema;
        j["type"] = "json";
        return j;
    }
    std::string json_property::to_deftemplate(const type &tp, bool is_static) const noexcept
    {
        std::string deftemplate;
        if (is_static)
            deftemplate = "(deftemplate " + tp.get_name() + "_has_" + get_name();
        else
            deftemplate = "(deftemplate " + tp.get_name() + "_" + get_name();
        deftemplate += " (slot item_id (type SYMBOL)) (slot " + get_name() + " (type STRING) (default \"\"))";
        deftemplate += ")";
        return deftemplate;
    }
    void json_property::set_value(FactBuilder *item_fact_builder, const json::json &value) const noexcept { FBPutSlotString(item_fact_builder, get_name().c_str(), value.dump().c_str()); }

    std::unique_ptr<property> make_property(const json::json &j)
    {
        if (j["type"] == "integer")
        {
            long min = std::numeric_limits<long>::min();
            if (j.contains("min"))
                min = j["min"];
            long max = std::numeric_limits<long>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<integer_property>(j["name"], j["description"], min, max);
        }
        if (j["type"] == "float")
        {
            double min = -std::numeric_limits<double>::max();
            if (j.contains("min"))
                min = j["min"];
            double max = std::numeric_limits<double>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<float_property>(j["name"], j["description"], min, max);
        }
        if (j["type"] == "string")
            return std::make_unique<string_property>(j["name"], j["description"]);
        if (j["type"] == "symbol")
            return std::make_unique<symbol_property>(j["name"], j["description"]);
        if (j["type"] == "json")
        {
            json::json schema = j["schema"];
            return std::make_unique<json_property>(j["name"], j["description"], std::move(schema));
        }
        throw std::runtime_error("Unknown property type");
    }
} // namespace coco
