#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "coco_rule.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

#ifdef BUILD_LISTENERS
#define CREATED_TYPE(tp) cc.created_type(tp)
#else
#define CREATED_TYPE(tp)
#endif

namespace coco
{
    type::type(coco &cc, std::string_view name, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data)) {}
    type::~type()
    {
        for (const auto &id : instances)
            cc.items.erase(id);
        auto dt = FindDeftemplate(cc.env, name.c_str());
        assert(dt);
        assert(DeftemplateIsDeletable(dt));
        [[maybe_unused]] auto undef_dt = Undeftemplate(dt, cc.env);
        assert(undef_dt);
    }

    void type::set_parents(json::json &&parents) noexcept
    {
        if (!is_a.empty())
        { // Remove existing inheritance rule..
            std::string rule_name = "is-a-" + name;
            for (const auto &parent : is_a)
                rule_name += "-" + parent.get().get_name();
            cc.rules.erase(rule_name);
            is_a.clear();
        }

        if (!parents.as_array().empty())
        {
            std::string rule_name = "is-a-" + name;
            std::unordered_set<std::string> unique_parents;
            for (const auto &parent : parents.as_array())
            { // Add parent type..
                std::string parent_name = parent.get<std::string>();
                if (unique_parents.emplace(parent_name).second)
                { // Not already present..
                    is_a.emplace_back(cc.get_type(parent_name));
                    rule_name += "-" + parent_name;
                }
            }
            std::string rule_content = "(defrule " + rule_name + " (" + get_name() + " (item_id ?id))\n  =>\n";
            for (const auto &parent : is_a)
                rule_content += "  (add_type ?id " + parent.get().get_name() + ")\n";
            rule_content += ')';
            cc.make_rule(rule_name, rule_content);
        }
    }

    void type::set_properties(json::json &&static_props, json::json &&dynamic_props) noexcept
    {
        if (!static_properties.empty() || !dynamic_properties.empty())
        { // Remove existing deftemplate..
            auto dt = FindDeftemplate(cc.env, name.c_str());
            assert(dt);
            assert(DeftemplateIsDeletable(dt));
            [[maybe_unused]] auto undef_dt = Undeftemplate(dt, cc.env);
            assert(undef_dt);
            static_properties.clear();
            dynamic_properties.clear();
        }

        for (auto &[name, prop] : static_props.as_object())
            static_properties.emplace(name, cc.get_property_type(prop["type"].get<std::string>()).new_instance(*this, false, name, prop));
        for (auto &[name, prop] : dynamic_props.as_object())
            dynamic_properties.emplace(name, cc.get_property_type(prop["type"].get<std::string>()).new_instance(*this, true, name, prop));

        std::string deftemplate = "(deftemplate " + get_name() + " (slot item_id (type SYMBOL))";
        for (const auto &[name, prop] : static_properties)
            deftemplate += " " + prop->get_slot_declaration();
        for (const auto &[name, prop] : dynamic_properties)
            deftemplate += " " + prop->get_slot_declaration();
        deftemplate += ')';
        LOG_TRACE(deftemplate);
        [[maybe_unused]] auto prop_dt = Build(cc.env, deftemplate.c_str());
        assert(prop_dt == BE_NO_ERROR);

        CREATED_TYPE(*this);
    }

    std::vector<std::reference_wrapper<item>> type::get_instances() const noexcept
    {
        std::vector<std::reference_wrapper<item>> res;
        for (const auto &id : instances)
            res.emplace_back(cc.get_item(id));
        return res;
    }
    void type::add_instance(item &itm) noexcept
    {
        itm.add_type(*this);
        instances.emplace(itm.get_id());
    }
    void type::remove_instance(item &itm) noexcept
    {
        itm.remove_type(*this);
        instances.erase(itm.get_id());
    }

    [[nodiscard]] json::json type::to_json() const noexcept
    {
        json::json j = json::json{{"name", name}};
        if (!data.as_object().empty())
            j["data"] = data;
        if (!is_a.empty())
        {
            json::json parents_json(json::json_type::array);
            for (const auto &parent : is_a)
                parents_json.push_back(parent.get().get_name());
            j["is_a"] = std::move(parents_json);
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
