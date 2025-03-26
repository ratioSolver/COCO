#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#include <cassert>

#ifdef BUILD_LISTENERS
#define NEW_TYPE() cc.new_type(*this)
#else
#define NEW_TYPE()
#endif

namespace coco
{
    type::type(coco &cc, std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotSymbol(type_fact_builder, "name", name.data());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        LOG_TRACE(cc.to_string(type_fact));
        FBDispose(type_fact_builder);
        set_parents(std::move(parents));
        for (auto &[name, prop] : static_props.as_object())
            static_properties.emplace(name, cc.get_property_type(static_cast<std::string>(prop["type"])).new_instance(*this, false, name, prop));
        for (auto &[name, prop] : dynamic_props.as_object())
            dynamic_properties.emplace(name, cc.get_property_type(static_cast<std::string>(prop["type"])).new_instance(*this, true, name, prop));
        Run(cc.env, -1);
        NEW_TYPE();
    }
    type::~type()
    {
        for (const auto &id : instances)
            cc.items.erase(id);
        for (auto &p : parent_facts)
            Retract(p.second);
        Retract(type_fact);
        Run(cc.env, -1);
    }

    void type::set_parents(std::vector<utils::ref_wrapper<const type>> &&parents) noexcept
    {
        // we retract the current parent facts (if any)..
        for (auto &p : parent_facts)
            Retract(p.second);
        parent_facts.clear();

        for (auto &p : parents)
        {
            FactBuilder *is_a_fact_builder = CreateFactBuilder(cc.env, "is_a");
            FBPutSlotSymbol(is_a_fact_builder, "type", name.c_str());
            FBPutSlotSymbol(is_a_fact_builder, "parent", p->name.c_str());
            auto parent_fact = FBAssert(is_a_fact_builder);
            assert(parent_fact);
            LOG_TRACE(cc.to_string(parent_fact));
            FBDispose(is_a_fact_builder);
            this->parents.emplace(p->name, p);
            parent_facts.emplace(p->name, parent_fact);
        }
        Run(cc.env, -1);
    }

    std::vector<utils::ref_wrapper<item>> type::get_instances() const noexcept
    {
        std::vector<utils::ref_wrapper<item>> res;
        for (const auto &id : instances)
            res.emplace_back(cc.get_item(id));
        return res;
    }

    item &type::make_item(std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val)
    {
        auto itm_ptr = utils::make_u_ptr<item>(*this, id, std::move(props), std::move(val));
        auto &itm = *itm_ptr;
        if (!cc.items.emplace(id.data(), std::move(itm_ptr)).second)
            throw std::invalid_argument("item `" + std::string(id) + "` already exists");
        instances.emplace(id);
        return itm;
    }

    [[nodiscard]] json::json type::to_json() const noexcept
    {
        json::json j = json::json{{"name", name.c_str()}};
        if (!data.as_object().empty())
            j["data"] = data;
        if (!parents.empty())
        {
            json::json j_pars(json::json_type::array);
            for (const auto &p : parents)
                j_pars.push_back(p.second->name.c_str());
            j["parents"] = j_pars;
        }
        if (!static_properties.empty())
        {
            json::json static_properties_json;
            for (const auto &[name, p] : static_properties)
                static_properties_json[name] = p->to_json();
            j["static_properties"] = std::move(static_properties_json);
        }
        if (!dynamic_properties.empty())
        {
            json::json dynamic_properties_json;
            for (const auto &[name, p] : dynamic_properties)
                dynamic_properties_json[name] = p->to_json();
            j["dynamic_properties"] = std::move(dynamic_properties_json);
        }
        return j;
    }
} // namespace coco
