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
    Environment *property_type::get_env() const noexcept { return cc.env; }

    bool_property_type::bool_property_type(coco &cc) noexcept : property_type(cc, bool_kw) {}
    std::unique_ptr<property> bool_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && (j["multiple"].get<bool>());
        std::optional<std::vector<bool>> default_value;
        if (j.contains("default"))
        {
            std::vector<bool> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(v.get<bool>());
            else
                def_v.emplace_back(j["default"].get<bool>());
            default_value = std::move(def_v);
        }
        return std::make_unique<bool_property>(*this, tp, dynamic, name, multiple, default_value);
    }

    int_property_type::int_property_type(coco &cc) noexcept : property_type(cc, int_kw) {}
    std::unique_ptr<property> int_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && j["multiple"].get<bool>();
        std::optional<std::vector<long>> default_value;
        if (j.contains("default"))
        {
            std::vector<long> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(v.get<long>());
            else
                def_v.emplace_back(j["default"].get<long>());
            default_value = std::move(def_v);
        }
        std::optional<long> min;
        if (j.contains("min"))
            min = j["min"].get<long>();
        std::optional<long> max;
        if (j.contains("max"))
            max = j["max"].get<long>();
        return std::make_unique<int_property>(*this, tp, dynamic, name, multiple, default_value, min, max);
    }

    float_property_type::float_property_type(coco &cc) noexcept : property_type(cc, float_kw) {}
    std::unique_ptr<property> float_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && j["multiple"].get<bool>();
        std::optional<std::vector<double>> default_value;
        if (j.contains("default"))
        {
            std::vector<double> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(v.get<double>());
            else
                def_v.emplace_back(j["default"].get<double>());
            default_value = std::move(def_v);
        }
        std::optional<double> min;
        if (j.contains("min"))
            min = j["min"].get<double>();
        std::optional<double> max;
        if (j.contains("max"))
            max = j["max"].get<double>();
        return std::make_unique<float_property>(*this, tp, dynamic, name, multiple, default_value, min, max);
    }

    string_property_type::string_property_type(coco &cc) noexcept : property_type(cc, string_kw) {}
    std::unique_ptr<property> string_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && j["multiple"].get<bool>();
        std::optional<std::vector<std::string>> default_value;
        if (j.contains("default"))
        {
            std::vector<std::string> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(v.get<std::string>());
            else
                def_v.emplace_back(j["default"].get<std::string>());
            default_value = std::move(def_v);
        }
        return std::make_unique<string_property>(*this, tp, dynamic, name, multiple, default_value);
    }

    symbol_property_type::symbol_property_type(coco &cc) noexcept : property_type(cc, symbol_kw) {}
    std::unique_ptr<property> symbol_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        bool multiple = j.contains("multiple") && j["multiple"].get<bool>();
        std::vector<std::string> values;
        if (j.contains("values"))
            for (const auto &v : j["values"].as_array())
                values.emplace_back(v.get<std::string>());
        std::optional<std::vector<std::string>> default_value;
        if (j.contains("default"))
        {
            std::vector<std::string> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(v.get<std::string>());
            else
                def_v.emplace_back(j["default"].get<std::string>());
            default_value = std::move(def_v);
        }
        return std::make_unique<symbol_property>(*this, tp, dynamic, name, multiple, std::move(values), default_value);
    }

    item_property_type::item_property_type(coco &cc) noexcept : property_type(cc, item_kw) {}
    std::unique_ptr<property> item_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        type &domain = cc.get_type(j["domain"].get<std::string>());
        bool multiple = j.contains("multiple") && j["multiple"].get<bool>();
        std::optional<std::vector<std::reference_wrapper<item>>> default_value;
        if (j.contains("default"))
        {
            std::vector<std::reference_wrapper<item>> def_v;
            if (multiple)
                for (const auto &v : j["default"].as_array())
                    def_v.emplace_back(cc.get_item(v.get<std::string>()));
            else
                def_v.emplace_back(cc.get_item(j["default"].get<std::string>()));
            default_value = std::move(def_v);
        }
        return std::make_unique<item_property>(*this, tp, dynamic, name, domain, multiple, default_value);
    }

    json_property_type::json_property_type(coco &cc) noexcept : property_type(cc, json_kw) {}
    std::unique_ptr<property> json_property_type::new_instance(type &tp, bool dynamic, std::string_view name, const json::json &j) noexcept
    {
        std::optional<json::json> schema;
        if (j.contains("schema"))
            schema = j["schema"];
        std::optional<json::json> default_value;
        if (j.contains("default"))
            default_value = j["default"];
        return std::make_unique<json_property>(*this, tp, dynamic, name, schema, default_value);
    }

    property::property(const property_type &pt, const type &tp, bool dynamic, std::string_view name) noexcept : pt(pt), tp(tp), dynamic(dynamic), name(name) {}
    property::~property()
    {
        auto dt = FindDeftemplate(pt.get_coco().env, get_deftemplate_name().c_str());
        assert(dt);
        assert(DeftemplateIsDeletable(dt));
        [[maybe_unused]] auto undef_dt = Undeftemplate(dt, pt.get_coco().env);
        assert(undef_dt);
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

    bool_property::bool_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::optional<std::vector<bool>> default_value) noexcept : property(pt, tp, dynamic, name), multiple(multiple), default_value(default_value)
    {
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
            for (const auto &val : *default_value)
                deftemplate += " " + std::string(val ? "TRUE" : "FALSE");
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool bool_property::validate(const json::json &j) const noexcept { return j.is_boolean(); }
    json::json bool_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = bool_kw;
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : *default_value)
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        return j;
    }
    json::json bool_property::fake() const noexcept
    {
        if (multiple) // Generate a random number of values.
        {
            std::uniform_int_distribution<std::size_t> dist_size(0, 5); // Random size between 0 and 5
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
            {
                std::bernoulli_distribution dist;
                j.push_back(dist(get_gen()));
            }
            return j;
        }
        else
        { // Generate a single value.
            std::bernoulli_distribution dist;
            return dist(get_gen());
        }
    }
    void bool_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_boolean());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_boolean(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, v.get<bool>() ? "TRUE" : "FALSE");
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_boolean());
            [[maybe_unused]] auto put_slot_err = FBPutSlotSymbol(property_fact_builder, name.data(), value.get<bool>() ? "TRUE" : "FALSE");
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void bool_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_boolean());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_boolean(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, v.get<bool>() ? "TRUE" : "FALSE");
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FMPutSlotMultifield(property_fact_modifier, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_boolean());
            [[maybe_unused]] auto put_slot_err = FMPutSlotSymbol(property_fact_modifier, name.data(), value.get<bool>() ? "TRUE" : "FALSE");
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }

    int_property::int_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::optional<std::vector<long>> default_value, std::optional<long> min, std::optional<long> max) noexcept : property(pt, tp, dynamic, name), multiple(multiple), default_value(default_value), min(min), max(max)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (";
        if (multiple)
            deftemplate += "multislot ";
        else
            deftemplate += "slot ";
        deftemplate += name.data();
        deftemplate += " (type INTEGER)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : *default_value)
                deftemplate += " " + std::to_string(val);
            deftemplate += ")";
        }
        if (min.has_value() || max.has_value())
        {
            deftemplate += " (range ";
            deftemplate += min.has_value() ? std::to_string(*min) : "?VARIABLE";
            deftemplate += ' ';
            deftemplate += max.has_value() ? std::to_string(*max) : "?VARIABLE";
            deftemplate += ')';
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool int_property::validate(const json::json &j) const noexcept
    {
        if (!j.is_integer())
            return false;
        long value = j;
        if ((min.has_value() && *min > value) || (max.has_value() && *max < value))
            return false;
        return true;
    }
    json::json int_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = int_kw;
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : *default_value)
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        if (min.has_value())
            j["min"] = *min;
        if (max.has_value())
            j["max"] = *max;
        return j;
    }
    json::json int_property::fake() const noexcept
    {
        if (multiple) // Generate a random number of values.
        {
            std::uniform_int_distribution<std::size_t> dist_size(0, 5); // Random size between 0 and 5
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
            {
                std::uniform_int_distribution<long> dist(min.has_value() ? *min : std::numeric_limits<long>::min(), max.has_value() ? *max : std::numeric_limits<long>::max());
                j.push_back(dist(get_gen()));
            }
            return j;
        }
        else
        { // Generate a single value.
            std::uniform_int_distribution<long> dist(min.has_value() ? *min : std::numeric_limits<long>::min(), max.has_value() ? *max : std::numeric_limits<long>::max());
            return dist(get_gen());
        }
    }
    void int_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_integer());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_integer(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendInteger(mfb, v.get<long>());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_integer());
            [[maybe_unused]] auto put_slot_err = FBPutSlotInteger(property_fact_builder, name.data(), value.get<long>());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void int_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_integer());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_integer(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendInteger(mfb, v.get<long>());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FMPutSlotMultifield(property_fact_modifier, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_integer());
            [[maybe_unused]] auto put_slot_err = FMPutSlotInteger(property_fact_modifier, name.data(), value.get<long>());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }

    float_property::float_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::optional<std::vector<double>> default_value, std::optional<double> min, std::optional<double> max) noexcept : property(pt, tp, dynamic, name), multiple(multiple), default_value(default_value), min(min), max(max)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (";
        if (multiple)
            deftemplate += "multislot ";
        else
            deftemplate += "slot ";
        deftemplate += name.data();
        deftemplate += " (type FLOAT)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : *default_value)
                deftemplate += " " + std::to_string(val);
            deftemplate += ")";
        }
        if (min.has_value() || max.has_value())
        {
            deftemplate += " (range ";
            deftemplate += min.has_value() ? std::to_string(*min) : "?VARIABLE";
            deftemplate += ' ';
            deftemplate += max.has_value() ? std::to_string(*max) : "?VARIABLE";
            deftemplate += ')';
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool float_property::validate(const json::json &j) const noexcept
    {
        if (!j.is_number())
            return false;
        double value = j;
        if ((min.has_value() && *min > value) || (max.has_value() && *max < value))
            return false;
        return true;
    }
    json::json float_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = float_kw;
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : *default_value)
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        if (min.has_value())
            j["min"] = *min;
        if (max.has_value())
            j["max"] = *max;
        return j;
    }
    json::json float_property::fake() const noexcept
    {
        if (multiple) // Generate a random number of values.
        {
            std::uniform_int_distribution<std::size_t> dist_size(0, 5); // Random size between 0 and 5
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
            {
                std::uniform_real_distribution<double> dist(min.has_value() ? *min : std::numeric_limits<double>::min(), max.has_value() ? *max : std::numeric_limits<double>::max());
                j.push_back(dist(get_gen()));
            }
            return j;
        }
        else
        { // Generate a single value.
            std::uniform_real_distribution<double> dist(min.has_value() ? *min : std::numeric_limits<double>::min(), max.has_value() ? *max : std::numeric_limits<double>::max());
            return dist(get_gen());
        }
    }
    void float_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_number());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_number(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendFloat(mfb, v.get<double>());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_number());
            [[maybe_unused]] auto put_slot_err = FBPutSlotFloat(property_fact_builder, name.data(), value.get<double>());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void float_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_number());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_number(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendFloat(mfb, v.get<double>());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FMPutSlotMultifield(property_fact_modifier, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_number());
            [[maybe_unused]] auto put_slot_err = FMPutSlotFloat(property_fact_modifier, name.data(), value.get<double>());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }

    string_property::string_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::optional<std::vector<std::string>> default_value) noexcept : property(pt, tp, dynamic, name), multiple(multiple), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (";
        if (multiple)
            deftemplate += "multislot ";
        else
            deftemplate += "slot ";
        deftemplate += name.data();
        deftemplate += " (type STRING)";
        if (default_value.has_value())
        {
            deftemplate += " (default";
            for (const auto &val : *default_value)
                deftemplate += " " + val;
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool string_property::validate(const json::json &j) const noexcept { return j.is_string(); }
    json::json string_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = string_kw;
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : *default_value)
                j_def_vals.push_back(val);
            j["default"] = j_def_vals;
        }
        return j;
    }
    json::json string_property::fake() const noexcept
    {
        if (multiple) // Generate a random number of values.
        {
            std::uniform_int_distribution<std::size_t> dist_size(0, 5); // Random size between 0 and 5
            json::json j(json::json_type::array);
            for (std::size_t i = 0; i < dist_size(get_gen()); ++i)
            {
                std::uniform_int_distribution<std::size_t> dist(32, 126);
                std::string str;
                for (std::size_t i = 0; i < dist(get_gen()); ++i)
                    str += static_cast<char>(dist(get_gen()));
                j.push_back(str);
            }
            return j;
        }
        else
        { // Generate a single value.
            std::uniform_int_distribution<std::size_t> dist(32, 126);
            std::string str;
            for (std::size_t i = 0; i < dist(get_gen()); ++i)
                str += static_cast<char>(dist(get_gen()));
            return str;
        }
    }
    void string_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_string());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_string(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendString(mfb, v.get<std::string>().c_str());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_string());
            [[maybe_unused]] auto put_slot_err = FBPutSlotString(property_fact_builder, name.data(), value.get<std::string>().c_str());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void string_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        [[maybe_unused]] auto put_slot_err = FMPutSlotString(property_fact_modifier, name.data(), value.get<std::string>().c_str());
        assert(put_slot_err == PSE_NO_ERROR);
    }

    symbol_property::symbol_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, bool multiple, std::vector<std::string> &&values, std::optional<std::vector<std::string>> default_value) noexcept : property(pt, tp, dynamic, name), multiple(multiple), values(values), default_value(default_value)
    {
        assert(!default_value.has_value() || values.empty() || std::all_of(default_value->begin(), default_value->end(), [this](const std::string &val)
                                                                           { return std::find(this->values.begin(), this->values.end(), val) != this->values.end(); }));
        assert(!default_value.has_value() || !multiple || default_value->size() <= 1);

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
            for (const auto &val : *default_value)
                deftemplate += " " + val;
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool symbol_property::validate(const json::json &j) const noexcept
    {
        if (multiple)
        {
            if (!j.is_array())
                return false;
            return std::all_of(j.as_array().begin(), j.as_array().end(), [this](const auto &val)
                               { return val.is_string(); });
        }
        else
            return j.is_string();
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
                j_vals.push_back(val);
            j["values"] = j_vals;
        }
        if (default_value.has_value())
        {
            auto j_def_vals = json::json(json::json_type::array);
            for (const auto &val : *default_value)
                j_def_vals.push_back(val);
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
                j.push_back(values[dist(get_gen())]);
            return j;
        }
        else // Generate a single value.
            return values[dist(get_gen())].c_str();
    }
    void symbol_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_string());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_string(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, v.get<std::string>().c_str());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_string());
            [[maybe_unused]] auto put_slot_err = FBPutSlotSymbol(property_fact_builder, name.data(), value.get<std::string>().c_str());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void symbol_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        assert(multiple == value.is_array() || value.is_string());
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_string(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, v.get<std::string>().c_str());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FMPutSlotMultifield(property_fact_modifier, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_string());
            [[maybe_unused]] auto put_slot_err = FMPutSlotSymbol(property_fact_modifier, name.data(), value.get<std::string>().c_str());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }

    item_property::item_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, const type &domain, bool multiple, std::optional<std::vector<std::reference_wrapper<item>>> default_value) noexcept : property(pt, tp, dynamic, name), domain(domain), multiple(multiple), default_value(default_value)
    {
        assert(!default_value.has_value() || default_value->empty() || std::all_of(default_value->begin(), default_value->end(), [&domain](const auto &val)
                                                                                   { 
                                                                                        std::queue<const type *> q;
                                                                                        q.push(&val.get().get_type());
                                                                                        while (!q.empty())
                                                                                        {
                                                                                            auto tp = q.front();
                                                                                            q.pop();
                                                                                            if (tp == &domain)
                                                                                                return true;
                                                                                            for (const auto &[_, p] : tp->get_parents())
                                                                                                q.push(&p.get());
                                                                                        }
                                                                                        return false; }));
        assert(!default_value.has_value() || !multiple || default_value->size() <= 1);

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
            for (const auto &val : *default_value)
                deftemplate += " " + val.get().get_id();
            deftemplate += ")";
        }
        deftemplate += ')';
        if (dynamic)
            deftemplate += " (slot timestamp (type INTEGER))";
        deftemplate += ')';

        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool item_property::validate(const json::json &j) const noexcept
    {
        if (multiple)
        {
            if (!j.is_array())
                return false;
            return std::all_of(j.as_array().begin(), j.as_array().end(), [this](const auto &val)
                               { return val.is_string(); });
        }
        else
            return j.is_string();
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
            for (const auto &val : *default_value)
                j_def_vals.push_back(val.get().get_id().c_str());
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
                j.push_back(dom[dist(get_gen())].get().get_id());
            return j;
        }
        else // Generate a single value.
            return dom[dist(get_gen())].get().get_id().c_str();
    }
    void item_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_string(); }));
            assert(!domain.get_instances().empty());
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [this](const json::json &v)
                               { return std::find_if(domain.get_instances().begin(), domain.get_instances().end(), [&v](const auto &itm)
                                                     { return itm.get().get_id() == v.get<std::string>(); }) != domain.get_instances().end(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, tp.get_coco().get_item(v.get<std::string>()).get_id().c_str());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FBPutSlotMultifield(property_fact_builder, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_string());
            assert(!domain.get_instances().empty());
            assert(std::find_if(domain.get_instances().begin(), domain.get_instances().end(), [&value](const auto &itm)
                                { return itm.get().get_id() == value.get<std::string>(); }) != domain.get_instances().end());
            assert(!tp.get_coco().get_item(value.get<std::string>()).get_id().empty());
            auto &itm = tp.get_coco().get_item(value.get<std::string>());
            [[maybe_unused]] auto put_slot_err = FBPutSlotSymbol(property_fact_builder, name.data(), itm.get_id().c_str());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }
    void item_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        if (value.is_array())
        {
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [](const json::json &v)
                               { return v.is_string(); }));
            assert(!domain.get_instances().empty());
            assert(std::all_of(value.as_array().begin(), value.as_array().end(), [this](const json::json &v)
                               { return std::find_if(domain.get_instances().begin(), domain.get_instances().end(), [&v](const auto &itm)
                                                     { return itm.get().get_id() == v.get<std::string>(); }) != domain.get_instances().end(); }));
            auto mfb = CreateMultifieldBuilder(get_env(), value.as_array().size());
            for (const auto &v : value.as_array())
                MBAppendSymbol(mfb, tp.get_coco().get_item(v.get<std::string>()).get_id().c_str());
            auto mf = MBCreate(mfb);
            [[maybe_unused]] auto put_slot_err = FMPutSlotMultifield(property_fact_modifier, name.data(), mf);
            assert(put_slot_err == PSE_NO_ERROR);
            ReleaseMultifield(get_env(), mf);
            MBDispose(mfb);
        }
        else
        {
            assert(value.is_string());
            assert(!domain.get_instances().empty());
            assert(std::find_if(domain.get_instances().begin(), domain.get_instances().end(), [&value](const auto &itm)
                                { return itm.get().get_id() == value.get<std::string>(); }) != domain.get_instances().end());
            assert(!tp.get_coco().get_item(value.get<std::string>()).get_id().empty());
            auto &itm = tp.get_coco().get_item(value.get<std::string>());
            [[maybe_unused]] auto put_slot_err = FMPutSlotSymbol(property_fact_modifier, name.data(), itm.get_id().c_str());
            assert(put_slot_err == PSE_NO_ERROR);
        }
    }

    json_property::json_property(const property_type &pt, const type &tp, bool dynamic, std::string_view name, std::optional<json::json> schema, std::optional<json::json> default_value) noexcept : property(pt, tp, dynamic, name), schema(schema), default_value(default_value)
    {
        std::string deftemplate = "(deftemplate " + get_deftemplate_name() + " (slot item_id (type SYMBOL)) (slot " + name.data();
        deftemplate += " (type STRING)";
        if (default_value.has_value())
        {
            std::string def;
            for (char ch : default_value->dump())
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
        [[maybe_unused]] auto prop_dt = Build(get_env(), deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);
    }
    bool json_property::validate(const json::json &j) const noexcept { return schema.has_value() ? json::validate(j, *schema, get_schemas()) : true; }
    json::json json_property::to_json() const noexcept
    {
        json::json j;
        j["type"] = json_kw;
        if (schema.has_value())
            j["schema"] = *schema;
        if (default_value.has_value())
            j["default"] = *default_value;
        return j;
    }
    json::json json_property::fake() const noexcept { return json::json(); }
    void json_property::set_value(FactBuilder *property_fact_builder, const json::json &value) const noexcept
    {
        [[maybe_unused]] auto put_slot_err = FBPutSlotString(property_fact_builder, name.data(), value.get<std::string>().c_str());
        assert(put_slot_err == PSE_NO_ERROR);
    }
    void json_property::set_value(FactModifier *property_fact_modifier, const json::json &value) const noexcept
    {
        [[maybe_unused]] auto put_slot_err = FMPutSlotString(property_fact_modifier, name.data(), value.get<std::string>().c_str());
        assert(put_slot_err == PSE_NO_ERROR);
    }
} // namespace coco
