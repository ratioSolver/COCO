#include "coco_item.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef BUILD_LISTENERS
#define CREATED_ITEM() cc.created_item(*this)
#define UPDATED_ITEM() cc.updated_item(*this)
#define NEW_DATA(data, timestamp) cc.new_data(*this, data, timestamp)
#else
#define CREATED_ITEM()
#define UPDATED_ITEM()
#define NEW_DATA(data, timestamp)
#endif

namespace coco
{
    item::item(coco &cc, std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val) noexcept : cc(cc), id(id), properties(std::move(props)), value(std::move(val)) { CREATED_ITEM(); }
    item::~item() noexcept
    {
        for (auto &tp : get_types())
            remove_type(tp.get());
    }

    bool item::has_type(const type &tp) const noexcept { return item_facts.find(tp.get_name()) != item_facts.end(); }

    std::vector<std::reference_wrapper<type>> item::get_types() const noexcept
    {
        std::vector<std::reference_wrapper<type>> res;
        for (const auto &[type_name, _] : item_facts)
            res.emplace_back(cc.get_type(type_name));
        return res;
    }

    void item::set_properties(json::json &&props)
    {
        for (auto item_fact : item_facts)
        {
            FactModifier *fact_modifier = CreateFactModifier(cc.env, item_fact.second);
            const auto &static_props = cc.get_type(item_fact.first).get_static_properties();
            for (const auto &[p_name, val] : props.as_object())
                if (auto prop = static_props.find(p_name); prop != static_props.end())
                {
                    LOG_TRACE("Updating property " + p_name + " for item " + id + " with value " + val.dump());
                    if (prop->second->validate(val))
                    {
                        prop->second->set_value(fact_modifier, val);
                        if (val.is_null())
                            properties.erase(p_name);
                        else
                            properties[p_name] = val;
                    }
                    else
                        LOG_WARN("Property " + p_name + " for item " + id + " is not valid");
                }
            auto updated_fact = FMModify(fact_modifier);
            [[maybe_unused]] auto fm_err = FMError(cc.env);
            assert(fm_err == FME_NO_ERROR);
            assert(updated_fact);
            RetainFact(updated_fact);
            ReleaseFact(item_fact.second);
            item_fact.second = updated_fact;
            FMDispose(fact_modifier);
        }

        UPDATED_ITEM();
    }

    void item::set_value(std::pair<json::json, std::chrono::system_clock::time_point> &&val)
    {
        if (!value.has_value())
            value = std::make_pair(json::json(), val.second);
        else
            value->second = val.second;
        for (auto &[tp_name, v_fs] : value_facts)
        {
            auto item_fact = item_facts.at(tp_name);
            FactModifier *fact_modifier = CreateFactModifier(cc.env, item_fact);
            const auto &dynamic_props = cc.get_type(tp_name).get_dynamic_properties();

            for (const auto &[p_name, j_val] : val.first.as_object())
                if (auto prop = dynamic_props.find(p_name); prop != dynamic_props.end())
                {
                    LOG_TRACE("Updating data " + p_name + " for item " + id + " with value " + j_val.dump());
                    if (prop->second->validate(j_val))
                        prop->second->set_value(fact_modifier, j_val);
                    else
                        LOG_WARN("Data " + p_name + " for item " + id + " is not valid");
                    if (auto f = v_fs.find(p_name); f != v_fs.end())
                    { // property already exists
                        if (j_val.is_null())
                        { // we retract the old property
                            LOG_TRACE("Retracting data " + p_name + " for item " + id);
                            ReleaseFact(f->second);
                            [[maybe_unused]] auto re_err = Retract(f->second);
                            assert(re_err == RE_NO_ERROR);
                            value->first.erase(p_name);
                            v_fs.erase(p_name);
                        }
                        else
                        { // we update the property
                            LOG_TRACE("Updating data " + p_name + " for item " + id + " with value " + j_val.dump());
                            FactModifier *property_fact_modifier = CreateFactModifier(cc.env, f->second);
                            prop->second->set_value(property_fact_modifier, j_val);
                            FMPutSlotInteger(property_fact_modifier, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(val.second.time_since_epoch()).count());
                            auto value_fact = FMModify(property_fact_modifier);
                            [[maybe_unused]] auto fm_err = FMError(cc.env);
                            assert(fm_err == FME_NO_ERROR);
                            assert(value_fact);
                            RetainFact(value_fact);
                            ReleaseFact(f->second);
                            FMDispose(property_fact_modifier);
                            value->first[p_name] = j_val;
                            f->second = value_fact;
                        }
                    }
                    else if (!j_val.is_null())
                    { // we create a new property
                        LOG_TRACE("Creating data " + p_name + " for item " + id + " with value " + j_val.dump());
                        FactBuilder *value_fact_builder = CreateFactBuilder(cc.env, prop->second->get_deftemplate_name().c_str());
                        FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                        prop->second->set_value(value_fact_builder, j_val);
                        FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(val.second.time_since_epoch()).count());
                        auto value_fact = FBAssert(value_fact_builder);
                        [[maybe_unused]] auto fb_err = FBError(cc.env);
                        assert(fb_err == FBE_NO_ERROR);
                        assert(value_fact);
                        RetainFact(value_fact);
                        LOG_TRACE(cc.to_string(value_fact));
                        FBDispose(value_fact_builder);
                        value->first[p_name] = j_val;
                        v_fs.emplace(p_name, value_fact);
                    }
                }

            auto updated_fact = FMModify(fact_modifier);
            [[maybe_unused]] auto fm_err = FMError(cc.env);
            assert(fm_err == FME_NO_ERROR);
            assert(updated_fact);
            RetainFact(updated_fact);
            ReleaseFact(item_fact);
            item_fact = updated_fact;
            FMDispose(fact_modifier);
        }

        NEW_DATA(val.first, val.second);
    }

    const property &item::get_property(std::string_view name) const
    {
        for (const auto &[tp_name, _] : item_facts)
        {
            const auto &tp = cc.get_type(tp_name);
            const auto &static_props = tp.get_static_properties();
            if (auto prop = static_props.find(name.data()); prop != static_props.end())
                return *prop->second;
            const auto &dynamic_props = tp.get_dynamic_properties();
            if (auto prop = dynamic_props.find(name.data()); prop != dynamic_props.end())
                return *prop->second;
        }
        throw std::invalid_argument("property `" + std::string(name) + "` does not exist for item `" + id + "`");
    }

    json::json item::to_json() const noexcept
    {
        json::json j_itm;
        if (!item_facts.empty())
        {
            json::json types(json::json_type::array);
            for (const auto &[type_name, _] : item_facts)
                types.push_back(type_name);
            j_itm["types"] = std::move(types);
        }
        if (!properties.as_object().empty())
            j_itm["properties"] = properties;
        if (value.has_value())
            j_itm["value"] = json::json{{"data", value->first}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(value->second.time_since_epoch()).count()}};
        return j_itm;
    }

    void item::add_type(const type &tp)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(cc.env, tp.get_name().c_str());
        FBPutSlotSymbol(item_fact_builder, "item_id", id.data());
        auto &static_props = tp.get_static_properties();
        for (const auto &[p_name, val] : properties.as_object())
            if (auto prop = static_props.find(p_name); prop != static_props.end())
            {
                if (prop->second->validate(val))
                    prop->second->set_value(item_fact_builder, val);
                else
                    LOG_WARN("Property " + p_name + " for item " + id.data() + " is not valid");
            }
        auto &dynamic_props = tp.get_dynamic_properties();
        if (value.has_value())
            for (const auto &[p_name, j_val] : value->first.as_object())
                if (auto prop = dynamic_props.find(p_name); prop != dynamic_props.end())
                {
                    if (prop->second->validate(j_val))
                        prop->second->set_value(item_fact_builder, j_val);
                    else
                        LOG_WARN("Dynamic property " + p_name + " for item " + id.data() + " is not valid");
                }
        auto item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        assert(item_fact);
        RetainFact(item_fact);
        LOG_TRACE(cc.to_string(item_fact));
        FBDispose(item_fact_builder);
        item_facts.emplace(tp.get_name(), item_fact);
        value_facts.emplace(tp.get_name(), std::map<std::string, Fact *>());
    }

    void item::remove_type(const type &tp)
    {
        for (auto &[_, tf] : value_facts.at(tp.get_name()))
        {
            ReleaseFact(tf);
            [[maybe_unused]] auto re_err = Retract(tf);
            assert(re_err == RE_NO_ERROR);
        }
        value_facts.erase(tp.get_name());

        auto it = item_facts.find(tp.get_name());
        assert(it != item_facts.end());
        ReleaseFact(it->second);
        [[maybe_unused]] auto re_err = Retract(it->second);
        assert(re_err == RE_NO_ERROR);
        item_facts.erase(it);
    }
} // namespace coco
