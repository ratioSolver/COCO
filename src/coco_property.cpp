#include "coco_core.hpp"
#include <stdexcept>
#include <algorithm>
#include <cassert>

namespace coco
{
    property::property(const type &tp, const std::string &name, const std::string &description) noexcept : tp(tp), name(name), description(description) {}
    json::json property::to_json() const noexcept { return json::json{{"description", description}}; }
    std::string property::to_deftemplate_name(bool is_dynamic) const noexcept
    {
        std::string type_name = tp.get_name();
        type_name.erase(std::remove(type_name.begin(), type_name.end(), ' '), type_name.end());
        std::string property_name = get_name();
        property_name.erase(std::remove(property_name.begin(), property_name.end(), ' '), property_name.end());
        if (is_dynamic)
            return type_name + "_has_" + property_name;
        return type_name + "_" + property_name;
    }

    boolean_property::boolean_property(const type &tp, const std::string &name, const std::string &description, std::optional<bool> default_value) noexcept : property(tp, name, description), default_value(default_value) {}
    bool boolean_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::boolean; }
    json::json boolean_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "boolean";
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string boolean_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (slot " + get_name() + " (type SYMBOL)";
        if (default_value.has_value())
        {
            if (default_value.value())
                deftemplate += " (default TRUE)";
            else
                deftemplate += " (default FALSE)";
        }
        deftemplate += "))";
        return deftemplate;
    }
    void boolean_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<bool>(value) ? "TRUE" : "FALSE"); }

    integer_property::integer_property(const type &tp, const std::string &name, const std::string &description, std::optional<long> default_value, long min, long max) noexcept : property(tp, name, description), default_value(default_value), min(min), max(max) {}
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
    std::string integer_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (slot " + get_name() + " (type INTEGER)";
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

    float_property::float_property(const type &tp, const std::string &name, const std::string &description, std::optional<double> default_value, double min, double max) noexcept : property(tp, name, description), default_value(default_value), min(min), max(max) {}
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
    std::string float_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (slot " + get_name() + " (type FLOAT)";
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

    string_property::string_property(const type &tp, const std::string &name, const std::string &description, std::optional<std::string> default_value) noexcept : property(tp, name, description), default_value(default_value) {}
    bool string_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json string_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "string";
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    std::string string_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (slot " + get_name() + " (type STRING)";
        if (default_value.has_value())
            deftemplate += " (default \"" + default_value.value() + "\")";
        deftemplate += "))";
        return deftemplate;
    }
    void string_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    symbol_property::symbol_property(const type &tp, const std::string &name, const std::string &description, std::optional<std::vector<std::string>> default_value, std::vector<std::string> values, bool multiple) noexcept : property(tp, name, description), default_value(default_value), values(std::move(values)), multiple(multiple)
    {
        assert(!default_value.has_value() || values.empty() || std::all_of(default_value.value().begin(), default_value.value().end(), [this](const std::string &val)
                                                                           { return std::find(this->values.begin(), this->values.end(), val) != this->values.end(); }));
        assert(!default_value.has_value() || !multiple || default_value.value().size() <= 1);
    }
    bool symbol_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json symbol_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "symbol";
        j["multiple"] = multiple;
        if (!values.empty())
        {
            auto j_vals = json::json(json::json_type::array);
            for (const auto &val : values)
                j_vals.push_back(val);
            j["values"] = j_vals;
        }
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : default_value.value())
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        return j;
    }
    std::string symbol_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (";
        if (multiple)
            deftemplate += "multislot";
        else
            deftemplate += "slot";
        deftemplate += " " + get_name() + " (type SYMBOL)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : default_value.value())
                deftemplate += " " + val;
            deftemplate += ")";
        }
        deftemplate += "))";
        return deftemplate;
    }
    void symbol_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    item_property::item_property(const type &tp, const std::string &name, const std::string &description, const type &domain, std::optional<std::vector<std::string>> default_value, std::vector<std::string> values, bool multiple) noexcept : property(tp, name, description), domain(domain), default_value(default_value), values(std::move(values)), multiple(multiple)
    {
        assert(!default_value.has_value() || values.empty() || std::all_of(default_value.value().begin(), default_value.value().end(), [this](const std::string &val)
                                                                           { return std::find(this->values.begin(), this->values.end(), val) != this->values.end(); }));
        assert(!default_value.has_value() || !multiple || default_value.value().size() <= 1);
    }
    bool item_property::validate(const json::json &j, const json::json &) const noexcept { return j.get_type() == json::json_type::string; }
    json::json item_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = "item";
        j["type_id"] = domain.get_id();
        j["multiple"] = multiple;
        if (!values.empty())
        {
            auto j_vals = json::json(json::json_type::array);
            for (const auto &val : values)
                j_vals.push_back(val);
            j["values"] = j_vals;
        }
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : default_value.value())
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        return j;
    }
    std::string item_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (";
        if (multiple)
            deftemplate += "multislot";
        else
            deftemplate += "slot";
        deftemplate += " " + get_name() + " (type SYMBOL)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : default_value.value())
                deftemplate += " " + val;
            deftemplate += ")";
        }
        deftemplate += "))";
        return deftemplate;
    }
    void item_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotSymbol(property_fact_builder, get_name().c_str(), static_cast<std::string>(value).c_str()); }

    json_property::json_property(const type &tp, const std::string &name, const std::string &description, json::json &&schema, std::optional<json::json> default_value) noexcept : property(tp, name, description), schema(std::move(schema)), default_value(default_value) {}
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
    std::string json_property::to_deftemplate(bool is_dynamic) const noexcept
    {
        std::string deftemplate = "(deftemplate " + to_deftemplate_name(is_dynamic) + " (slot item_id (type SYMBOL))";
        if (is_dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += " (slot " + get_name() + " (type STRING)";
        if (default_value.has_value())
            deftemplate += " (default \"" + default_value.value().dump() + "\")";
        deftemplate += "))";
        return deftemplate;
    }
    void json_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, get_name().c_str(), value.dump().c_str()); }

    std::unique_ptr<property> make_property(const type &tp, const std::string &name, const json::json &j)
    {
        if (j["type"] == "boolean")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<bool> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<boolean_property>(tp, name, description, default_value);
        }
        else if (j["type"] == "integer")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<long> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            long min = std::numeric_limits<long>::min();
            if (j.contains("min"))
                min = j["min"];
            long max = std::numeric_limits<long>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<integer_property>(tp, name, description, default_value, min, max);
        }
        else if (j["type"] == "float")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<double> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            double min = -std::numeric_limits<double>::max();
            if (j.contains("min"))
                min = j["min"];
            double max = std::numeric_limits<double>::max();
            if (j.contains("max"))
                max = j["max"];
            return std::make_unique<float_property>(tp, name, description, default_value, min, max);
        }
        else if (j["type"] == "string")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<std::string> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<string_property>(tp, name, description, default_value);
        }
        else if (j["type"] == "symbol")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<std::vector<std::string>> default_value;
            if (j.contains("default"))
            {
                default_value = std::vector<std::string>();
                for (const auto &val : j["default"].as_array())
                    default_value.value().push_back(val);
            }
            std::vector<std::string> values;
            if (j.contains("values"))
                for (const auto &val : j["values"].as_array())
                    values.push_back(val);
            bool multiple = false;
            if (j.contains("multiple"))
                multiple = j["multiple"];
            return std::make_unique<symbol_property>(tp, name, description, default_value, values, multiple);
        }
        else if (j["type"] == "item")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            std::optional<std::vector<std::string>> default_value;
            if (j.contains("default"))
            {
                default_value = std::vector<std::string>();
                for (const auto &val : j["default"].as_array())
                    default_value.value().push_back(val);
            }
            std::vector<std::string> values;
            if (j.contains("values"))
                for (const auto &val : j["values"].as_array())
                    values.push_back(val);
            bool multiple = false;
            if (j.contains("multiple"))
                multiple = j["multiple"];
            return std::make_unique<item_property>(tp, name, description, tp.get_core().get_type(j["type_id"]), default_value, values, multiple);
        }
        else if (j["type"] == "json")
        {
            std::string description = j.contains("description") ? j["description"] : "";
            json::json schema = j["schema"];
            std::optional<json::json> default_value;
            if (j.contains("default"))
                default_value = j["default"];
            return std::make_unique<json_property>(tp, name, description, std::move(schema), default_value);
        }
        throw std::runtime_error("Unknown property type");
    }
} // namespace coco
