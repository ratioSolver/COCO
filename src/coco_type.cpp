#include "coco_type.hpp"
#include "coco.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "logging.hpp"
#include <queue>
#include <cassert>

#ifdef BUILD_LISTENERS
#define CREATED_TYPE() cc.created_type(*this)
#else
#define CREATED_TYPE()
#endif

namespace coco
{
    type::type(coco &cc, std::string_view name, std::vector<std::reference_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotSymbol(type_fact_builder, "name", name.data());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        RetainFact(type_fact);
        LOG_TRACE(cc.to_string(type_fact));
        FBDispose(type_fact_builder);
        set_parents(std::move(parents));
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

        CREATED_TYPE();
    }
    type::~type()
    {
        for (const auto &id : instances)
            cc.items.erase(id);
        auto dt = FindDeftemplate(cc.env, name.c_str());
        assert(dt);
        assert(DeftemplateIsDeletable(dt));
        [[maybe_unused]] auto undef_dt = Undeftemplate(dt, cc.env);
        assert(undef_dt);

        for (auto &p : parent_facts)
        {
            ReleaseFact(p.second);
            [[maybe_unused]] auto re_err = Retract(p.second);
            assert(re_err == RE_NO_ERROR);
        }
        ReleaseFact(type_fact);
        [[maybe_unused]] auto re_err = Retract(type_fact);
        assert(re_err == RE_NO_ERROR);
    }

    const std::map<std::string, std::reference_wrapper<const property>> type::get_all_static_properties() const noexcept
    {
        std::map<std::string, std::reference_wrapper<const property>> static_props;
        std::queue<const type *> q;
        q.push(this);

        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->static_properties)
                static_props.emplace(p_name, *p);

            for (const auto &tp : t->parents)
                q.push(&tp.second.get());
        }
        return static_props;
    }

    const std::map<std::string, std::reference_wrapper<const property>> type::get_all_dynamic_properties() const noexcept
    {
        std::map<std::string, std::reference_wrapper<const property>> dynamic_props;
        std::queue<const type *> q;
        q.push(this);

        while (!q.empty())
        {
            const type *t = q.front();
            q.pop();

            for (const auto &[p_name, p] : t->dynamic_properties)
                dynamic_props.emplace(p_name, *p);

            for (const auto &tp : t->parents)
                q.push(&tp.second.get());
        }
        return dynamic_props;
    }

    void type::set_parents(std::vector<std::reference_wrapper<const type>> &&parents) noexcept
    {
        // we retract the current parent facts (if any)..
        for (auto &p : parent_facts)
        {
            ReleaseFact(p.second);
            [[maybe_unused]] auto re_err = Retract(p.second);
            assert(re_err == RE_NO_ERROR);
        }
        parent_facts.clear();

        for (auto &p : parents)
        {
            FactBuilder *is_a_fact_builder = CreateFactBuilder(cc.env, "is_a");
            FBPutSlotSymbol(is_a_fact_builder, "type", name.c_str());
            FBPutSlotSymbol(is_a_fact_builder, "parent", p.get().name.c_str());
            auto parent_fact = FBAssert(is_a_fact_builder);
            assert(parent_fact);
            RetainFact(parent_fact);
            LOG_TRACE(cc.to_string(parent_fact));
            FBDispose(is_a_fact_builder);
            this->parents.emplace(p.get().name, p);
            parent_facts.emplace(p.get().name, parent_fact);
        }
    }

    std::vector<std::reference_wrapper<item>> type::get_instances() const noexcept
    {
        std::vector<std::reference_wrapper<item>> res;
        for (const auto &id : instances)
            res.emplace_back(cc.get_item(id));
        return res;
    }

    item &type::make_item(std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val)
    {
        auto itm_ptr = std::make_unique<item>(*this, id, std::move(props), std::move(val));
        auto &itm = *itm_ptr;
        if (!cc.items.emplace(id.data(), std::move(itm_ptr)).second)
            throw std::invalid_argument("item `" + std::string(id) + "` already exists");
        instances.emplace(id);
        return itm;
    }

    [[nodiscard]] json::json type::to_json() const noexcept
    {
        json::json j = json::json{{"name", name}};
        if (!data.as_object().empty())
            j["data"] = data;
        if (!parents.empty())
        {
            json::json j_pars(json::json_type::array);
            for (const auto &p : parents)
                j_pars.push_back(p.second.get().name);
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
