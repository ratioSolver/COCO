#include "coco_item.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef BUILD_LISTENERS
#define CREATED_ITEM() tp.get_coco().created_item(*this)
#define UPDATED_ITEM() tp.get_coco().updated_item(*this)
#define NEW_DATA(data, timestamp) tp.get_coco().new_data(*this, data, timestamp)
#else
#define CREATED_ITEM()
#define UPDATED_ITEM()
#define NEW_DATA(data, timestamp)
#endif

namespace coco
{
    item::item(type &tp, std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val) noexcept : tp(tp), id(id), properties(props)
    {
        FactBuilder *item_fact_builder = CreateFactBuilder(tp.get_coco().env, "item");
        FBPutSlotSymbol(item_fact_builder, "id", id.data());
        item_fact = FBAssert(item_fact_builder);
        assert(item_fact);
        RetainFact(item_fact);
        LOG_TRACE(tp.get_coco().to_string(item_fact));
        FBDispose(item_fact_builder);

        FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(tp.get_coco().env, "instance_of");
        FBPutSlotSymbol(is_instance_of_fact_builder, "id", id.data());
        FBPutSlotSymbol(is_instance_of_fact_builder, "type", tp.get_name().c_str());
        is_instance_of = FBAssert(is_instance_of_fact_builder);
        assert(is_instance_of);
        RetainFact(is_instance_of);
        LOG_TRACE(tp.get_coco().to_string(is_instance_of));
        FBDispose(is_instance_of_fact_builder);

        CREATED_ITEM();

        set_properties(std::move(props));
        if (val.has_value())
            set_value(std::move(*val));
    }
    item::~item() noexcept
    {
        for (auto &v : value_facts)
        {
            ReleaseFact(v.second);
            [[maybe_unused]] auto re_err = Retract(v.second);
            assert(re_err == RE_NO_ERROR);
        }
        for (auto &p : properties_facts)
        {
            ReleaseFact(p.second);
            [[maybe_unused]] auto re_err = Retract(p.second);
            assert(re_err == RE_NO_ERROR);
        }
        ReleaseFact(is_instance_of);
        [[maybe_unused]] auto re_in_err = Retract(is_instance_of);
        assert(re_in_err == RE_NO_ERROR);
        ReleaseFact(item_fact);
        [[maybe_unused]] auto re_it_err = Retract(item_fact);
        assert(re_it_err == RE_NO_ERROR);
    }

    void item::set_properties(json::json &&props)
    {
        std::map<std::string, std::reference_wrapper<const property>> static_props = tp.get_all_static_properties();

        for (const auto &[p_name, val] : props.as_object())
            if (auto prop = static_props.find(p_name); prop != static_props.end())
            {
                if (auto f = value_facts.find(p_name); f != value_facts.end())
                {
                    if (val.is_null())
                    { // we retract the old property
                        LOG_TRACE("Retracting property " + p_name + " for item " + id);
                        ReleaseFact(f->second);
                        [[maybe_unused]] auto re_err = Retract(f->second);
                        assert(re_err == RE_NO_ERROR);
                        properties.erase(p_name);
                        value_facts.erase(p_name);
                    }
                    else if (prop->second.get().validate(val))
                    { // we update the property
                        LOG_TRACE("Updating property " + p_name + " for item " + id + " with value " + val.dump());
                        FactModifier *property_fact_modifier = CreateFactModifier(tp.get_coco().env, f->second);
                        prop->second.get().get_property_type().set_value(property_fact_modifier, p_name, val);
                        auto property_fact = FMModify(property_fact_modifier);
                        [[maybe_unused]] auto fm_err = FMError(tp.get_coco().env);
                        assert(fm_err == FME_NO_ERROR);
                        assert(property_fact);
                        RetainFact(property_fact);
                        ReleaseFact(f->second);
                        FMDispose(property_fact_modifier);
                        properties[p_name] = val;
                        value_facts[p_name] = property_fact;
                    }
                    else
                        LOG_WARN("Property " + p_name + " for item " + id + " is not valid");
                }
                else if (!val.is_null() && prop->second.get().validate(val))
                { // we create a new property
                    LOG_TRACE("Creating property " + p_name + " for item " + id + " with value " + val.dump());
                    FactBuilder *property_fact_builder = CreateFactBuilder(tp.get_coco().env, prop->second.get().get_deftemplate_name().c_str());
                    FBPutSlotSymbol(property_fact_builder, "item_id", id.c_str());
                    prop->second.get().get_property_type().set_value(property_fact_builder, p_name, val);
                    auto property_fact = FBAssert(property_fact_builder);
                    [[maybe_unused]] auto fb_err = FBError(tp.get_coco().env);
                    assert(fb_err == FBE_NO_ERROR);
                    assert(property_fact);
                    RetainFact(property_fact);
                    LOG_TRACE(tp.get_coco().to_string(property_fact));
                    FBDispose(property_fact_builder);
                    properties[p_name] = val;
                    properties_facts.emplace(p_name, property_fact);
                }
                else
                    LOG_WARN("Property " + p_name + " for item " + id + " is not valid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have static property " + p_name);
        UPDATED_ITEM();
    }

    void item::set_value(std::pair<json::json, std::chrono::system_clock::time_point> &&val)
    {
        std::map<std::string, std::reference_wrapper<const property>> dynamic_props = tp.get_all_dynamic_properties();

        if (!value.has_value())
            value = std::make_pair(json::json(), val.second);
        else
            value->second = val.second;

        for (const auto &[p_name, j_val] : val.first.as_object())
            if (auto prop = dynamic_props.find(p_name); prop != dynamic_props.end())
            {
                if (auto f = value_facts.find(p_name); f != value_facts.end())
                {
                    if (j_val.is_null())
                    { // we retract the old property
                        LOG_TRACE("Retracting data " + p_name + " for item " + id);
                        ReleaseFact(f->second);
                        [[maybe_unused]] auto re_err = Retract(f->second);
                        assert(re_err == RE_NO_ERROR);
                        value->first.erase(p_name);
                        value_facts.erase(p_name);
                    }
                    else if (prop->second.get().validate(j_val))
                    { // we update the property
                        LOG_TRACE("Updating data " + p_name + " for item " + id + " with value " + j_val.dump());
                        LOG_DEBUG(DeftemplatePPForm(FactDeftemplate(f->second)));
                        LOG_DEBUG("Fact: " + tp.get_coco().to_string(f->second));
                        FactModifier *property_fact_modifier = CreateFactModifier(tp.get_coco().env, f->second);
                        prop->second.get().get_property_type().set_value(property_fact_modifier, p_name, j_val);
                        FMPutSlotInteger(property_fact_modifier, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(val.second.time_since_epoch()).count());
                        auto value_fact = FMModify(property_fact_modifier);
                        [[maybe_unused]] auto fm_err = FMError(tp.get_coco().env);
                        assert(fm_err == FME_NO_ERROR);
                        assert(value_fact);
                        RetainFact(value_fact);
                        ReleaseFact(f->second);
                        FMDispose(property_fact_modifier);
                        value->first[p_name] = j_val;
                        value_facts[p_name] = value_fact;
                    }
                    else
                        LOG_WARN("Data " + p_name + " for item " + id + " is not valid");
                }
                else if (!j_val.is_null() && prop->second.get().validate(j_val))
                { // we create a new property
                    LOG_TRACE("Creating data " + p_name + " for item " + id + " with value " + j_val.dump());
                    FactBuilder *value_fact_builder = CreateFactBuilder(tp.get_coco().env, prop->second.get().get_deftemplate_name().c_str());
                    FBPutSlotSymbol(value_fact_builder, "item_id", id.c_str());
                    prop->second.get().get_property_type().set_value(value_fact_builder, p_name, j_val);
                    FBPutSlotInteger(value_fact_builder, "timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(val.second.time_since_epoch()).count());
                    auto value_fact = FBAssert(value_fact_builder);
                    [[maybe_unused]] auto fb_err = FBError(tp.get_coco().env);
                    assert(fb_err == FBE_NO_ERROR);
                    assert(value_fact);
                    RetainFact(value_fact);
                    LOG_TRACE(tp.get_coco().to_string(value_fact));
                    FBDispose(value_fact_builder);
                    value->first[p_name] = j_val;
                    value_facts.emplace(p_name, value_fact);
                }
                else
                    LOG_WARN("Data " + p_name + " for item " + id + " is not valid");
            }
            else
                LOG_WARN("Type " + tp.get_name() + " does not have dynamic property " + p_name);
        NEW_DATA(val.first, val.second);
    }

    json::json item::to_json() const noexcept
    {
        json::json j_itm{{"type", tp.get_name()}};
        if (!properties.as_object().empty())
            j_itm["properties"] = properties;
        if (value.has_value())
            j_itm["value"] = json::json{{"data", value->first}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(value->second.time_since_epoch()).count()}};
        return j_itm;
    }
} // namespace coco
