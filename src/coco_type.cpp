#include "coco_type.hpp"
#include "coco_core.hpp"
#include <cassert>

namespace coco
{
    type::type(coco_core &cc, const std::string &id, const std::string &name, const std::string &description, json::json &&props) noexcept : cc(cc), id(id), name(name), description(description), properties(std::move(props))
    {
        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotSymbol(type_fact_builder, "id", id.c_str());
        FBPutSlotString(type_fact_builder, "name", name.c_str());
        FBPutSlotString(type_fact_builder, "description", description.c_str());
        type_fact = FBAssert(type_fact_builder);
        assert(type_fact);
        FBDispose(type_fact_builder);
    }
    type::~type() noexcept
    {
        for (auto &p : parent_facts)
            Retract(p.second);
        Retract(type_fact);
    }

    bool type::is_assignable_from(const type &other) const noexcept
    {
        if (this == &other)
            return true;
        std::queue<const type *> q;
        q.push(this);
        while (!q.empty())
        {
            auto tp = q.front();
            q.pop();
            for (auto &p : tp->parents)
            {
                if (&p.second.get() == &other)
                    return true;
                q.push(&p.second.get());
            }
        }
        return false;
    }

    void type::set_name(const std::string &name) noexcept
    {
        this->name = name;
        FactModifier *type_fm = CreateFactModifier(cc.env, type_fact);
        FMPutSlotString(type_fm, "name", name.c_str());
        type_fact = FMModify(type_fm);
        FMDispose(type_fm);
    }

    void type::set_description(const std::string &description) noexcept
    {
        this->description = description;
        FactModifier *type_fm = CreateFactModifier(cc.env, type_fact);
        FMPutSlotString(type_fm, "description", description.c_str());
        type_fact = FMModify(type_fm);
        FMDispose(type_fm);
    }

    void type::set_properties(json::json &&props) noexcept { properties = props; }

    void type::set_parents(const std::vector<std::reference_wrapper<const type>> &parents) noexcept
    {
        for (auto &p : parent_facts)
            Retract(p.second);
        parent_facts.clear();
        this->parents.clear();
        for (auto &p : parents)
        {
            FactBuilder *is_a_fact_builder = CreateFactBuilder(cc.env, "is_a");
            FBPutSlotSymbol(is_a_fact_builder, "type_id", get_id().c_str());
            FBPutSlotSymbol(is_a_fact_builder, "parent_id", p.get().get_id().c_str());
            auto parent_fact = FBAssert(is_a_fact_builder);
            assert(parent_fact);
            FBDispose(is_a_fact_builder);
            this->parents.emplace(p.get().name, p);
            parent_facts.emplace(p.get().name, parent_fact);
        }
    }

    void type::set_static_properties(std::vector<std::unique_ptr<property>> &&props) noexcept
    {
        for (auto &p : static_properties)
            Undeftemplate(FindDeftemplate(cc.env, p.second->to_deftemplate_name(false).c_str()), cc.env);
        static_properties.clear();
        for (auto &p : props)
        {
            Build(cc.env, p->to_deftemplate(false).c_str());
            static_properties.emplace(p->get_name(), std::move(p));
        }
    }

    void type::set_dynamic_properties(std::vector<std::unique_ptr<property>> &&props) noexcept
    {
        for (auto &p : dynamic_properties)
            Undeftemplate(FindDeftemplate(cc.env, p.second->to_deftemplate_name().c_str()), cc.env);
        dynamic_properties.clear();
        for (auto &p : props)
        {
            Build(cc.env, p->to_deftemplate(true).c_str());
            dynamic_properties.emplace(p->get_name(), std::move(p));
        }
    }
} // namespace coco