#include "coco_property.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <algorithm>
#include <queue>
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

    symbol_property_type::symbol_property_type(coco &cc) noexcept : property_type(cc, symbol_kw) {}
    utils::u_ptr<property> symbol_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && static_cast<bool>(j["multiple"]);
        std::vector<std::string> values;
        if (j.contains("values"))
            for (const auto &v : j["values"].as_array())
                values.emplace_back(static_cast<std::string>(v));
        std::optional<std::vector<std::string>> default_value;
        if (j.contains("default"))
        {
            std::vector<std::string> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(static_cast<std::string>(v));
            def_v.emplace_back(static_cast<std::string>(j["default"]));
            default_value = std::move(def_v);
        }
        return utils::make_u_ptr<symbol_property>(*this, tp, dynamic, name, multiple, std::move(values), default_value);
    }
    void symbol_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, name.data(), static_cast<std::string>(value).c_str()); }

    item_property_type::item_property_type(coco &cc) noexcept : property_type(cc, item_kw) {}
    utils::u_ptr<property> item_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        type &domain = cc.get_type(static_cast<std::string>(j["domain"]));
        bool multiple = j.contains("multiple") && static_cast<bool>(j["multiple"]);
        std::optional<std::vector<utils::ref_wrapper<item>>> default_value;
        if (j.contains("default"))
        {
            std::vector<utils::ref_wrapper<item>> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(cc.get_item(static_cast<std::string>(v)));
            else
                def_v.emplace_back(cc.get_item(static_cast<std::string>(j["default"])));
            default_value = std::move(def_v);
        }
        return utils::make_u_ptr<item_property>(*this, tp, dynamic, name, domain, multiple, default_value);
    }
    void item_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, name.data(), static_cast<std::string>(value).c_str()); }

    json_property_type::json_property_type(coco &cc) noexcept : property_type(cc, json_kw) {}
    utils::u_ptr<property> json_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<json::json> schema;
        if (j.contains("schema"))
            schema = j["schema"];
        std::optional<json::json> default_value;
        if (j.contains("default"))
            default_value = j["default"];
        return utils::make_u_ptr<json_property>(*this, tp, dynamic, name, schema, default_value);
    }
    void json_property_type::set_value(FactBuilder *property_fact_builder, std::string_view name, const json::json &value) const noexcept { FBPutSlotString(property_fact_builder, name.data(), static_cast<std::string>(value).c_str()); }

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
    std::mt19937 &property::get_gen() const noexcept { return pt.get_coco().gen; }

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
    json::json bool_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = bool_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    json::json bool_property::fake() const noexcept
    {
        std::bernoulli_distribution dist;
        return dist(get_gen());
    }

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
    json::json int_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = int_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        if (min.has_value())
            j["min"] = min.value();
        if (max.has_value())
            j["max"] = max.value();
        return j;
    }
    json::json int_property::fake() const noexcept
    {
        std::uniform_int_distribution<long> dist(min.has_value() ? min.value() : std::numeric_limits<long>::min(), max.has_value() ? max.value() : std::numeric_limits<long>::max());
        return dist(get_gen());
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
    json::json float_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = float_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        if (min.has_value())
            j["min"] = min.value();
        if (max.has_value())
            j["max"] = max.value();
        return j;
    }
    json::json float_property::fake() const noexcept
    {
        std::uniform_real_distribution<double> dist(min.has_value() ? min.value() : std::numeric_limits<double>::min(), max.has_value() ? max.value() : std::numeric_limits<double>::max());
        return dist(get_gen());
    }

    string_property::string_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<std::string> default_value) noexcept : property(pt, tp, dynamic, name), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type STRING)";
        if (default_value.has_value())
            deftemplate += " (default " + default_value.value() + ")";
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool string_property::validate(const json::json &j) const noexcept { return j.get_type() == json::json_type::string; }
    json::json string_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = string_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    json::json string_property::fake() const noexcept
    {
        std::uniform_int_distribution<std::size_t> dist(0, 100);
        std::string str;
        for (std::size_t i = 0; i < dist(get_gen()); ++i)
            str += static_cast<char>(dist(get_gen()));
        return str;
    }

    symbol_property::symbol_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::vector<std::string> &&values, std::optional<std::vector<std::string>> default_value) noexcept : property(pt, tp, dynamic, name), multiple(multiple), values(values), default_value(default_value)
    {
        assert(!default_value.has_value() || values.empty() || std::all_of(default_value.value().begin(), default_value.value().end(), [this](const std::string &val)
                                                                           { return std::find(this->values.begin(), this->values.end(), val) != this->values.end(); }));
        assert(!default_value.has_value() || !multiple || default_value.value().size() <= 1);

        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (";
        if (multiple)
            deftemplate += "multislot ";
        else
            deftemplate += "slot ";
        deftemplate += name.data();
        deftemplate += " (type SYMBOL)";
        if (!values.empty())
        {
            deftemplate += " (allowed-values";
            for (const auto &val : values)
                deftemplate += " " + val;
            deftemplate += ")";
        }
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : default_value.value())
                deftemplate += " " + val;
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool symbol_property::validate(const json::json &j) const noexcept
    {
        if (multiple)
        {
            if (j.get_type() != json::json_type::array)
                return false;
            return std::all_of(j.as_array().begin(), j.as_array().end(), [this](const auto &val)
                               { return val.get_type() == json::json_type::string; });
        }
        else
            return j.get_type() == json::json_type::string;
    }
    json::json symbol_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = symbol_kw;
        j["multiple"] = multiple;
        if (!values.empty())
        {
            auto j_vals = json::json(json::json_type::array);
            for (const auto &val : values)
                j_vals.push_back(val.c_str());
            j["values"] = j_vals;
        }
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : default_value.value())
                j_def_vals.push_back(val.c_str());
            j["default"] = j_def_vals;
        }
        return j;
    }
    json::json symbol_property::fake() const noexcept
    {
        std::uniform_int_distribution<std::size_t> dist(0, values.size() - 1);
        if (multiple)
        { // Generate a random number of values.
            std::uniform_int_distribution<std::size_t> dist_size(0, values.size());
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
                j.push_back(values[dist(get_gen())].c_str());
            return j;
        }
        else // Generate a single value.
            return values[dist(get_gen())].c_str();
    }

    item_property::item_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, const type &domain, bool multiple, std::optional<std::vector<utils::ref_wrapper<item>>> default_value) noexcept : property(pt, tp, dynamic, name), domain(domain), multiple(multiple), default_value(default_value)
    {
        assert(!default_value.has_value() || default_value.value().empty() || std::all_of(default_value.value().begin(), default_value.value().end(), [&domain](const auto &val)
                                                                                          { 
                                                                                            std::queue<const type *> q;
                                                                                            q.push(&val->get_type());
                                                                                            while (!q.empty())
                                                                                            {
                                                                                                auto tp = q.front();
                                                                                                q.pop();
                                                                                                if (tp == &domain)
                                                                                                    return true;
                                                                                                for (const auto &[_, p] : tp->get_parents())
                                                                                                    q.push(&*p);
                                                                                            }
                                                                                            return false; }));
        assert(!default_value.has_value() || !multiple || default_value.value().size() <= 1);

        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (";
        if (multiple)
            deftemplate += "multislot ";
        else
            deftemplate += "slot ";
        deftemplate += name.data();
        deftemplate += " (type SYMBOL)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : default_value.value())
                deftemplate += " " + val->get_id();
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool item_property::validate(const json::json &j) const noexcept
    {
        if (multiple)
        {
            if (j.get_type() != json::json_type::array)
                return false;
            return std::all_of(j.as_array().begin(), j.as_array().end(), [this](const auto &val)
                               { return val.get_type() == json::json_type::string; });
        }
        else
            return j.get_type() == json::json_type::string;
    }
    json::json item_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = item_kw;
        j["domain"] = domain.get_name();
        j["multiple"] = multiple;
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : default_value.value())
                j_def_vals.push_back(val->get_id().c_str());
            j["default"] = j_def_vals;
        }
        return j;
    }
    json::json item_property::fake() const noexcept
    {
        const auto dom = domain.get_instances();
        std::uniform_int_distribution<std::size_t> dist(0, dom.size() - 1);
        if (multiple)
        { // Generate a random number of values.
            std::uniform_int_distribution<std::size_t> dist_size(0, dom.size());
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
                j.push_back(dom[dist(get_gen())]->get_id().c_str());
            return j;
        }
        else // Generate a single value.
            return dom[dist(get_gen())]->get_id().c_str();
    }

    json_property::json_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<json::json> schema, std::optional<json::json> default_value) noexcept : property(pt, tp, dynamic, name), schema(schema), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type STRING)";
        if (default_value.has_value())
        {
            std::string def;
            for (char ch : default_value.value().dump())
                if (ch == '"')
                    def += "\\\"";
                else if (ch == '\\')
                    def += "\\\\";
                else
                    def += ch;
            deftemplate += " (default \"" + def + "\")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        Build(get_env(), deftemplate.c_str());
    }
    bool json_property::validate(const json::json &j) const noexcept { return schema.has_value() ? json::validate(j, schema.value(), get_schemas()) : true; }
    json::json json_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = json_kw;
        if (schema.has_value())
            j["schema"] = schema.value();
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
    json::json json_property::fake() const noexcept { return json::json(); }
} // namespace coco
