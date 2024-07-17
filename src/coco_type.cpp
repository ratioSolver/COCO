#include "coco_type.hpp"
#include "coco_core.hpp"

namespace coco
{
    type::type(coco_core &cc, const std::string &id, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties) noexcept : cc(cc), id(id), name(name), description(description)
    {
        for (auto &p : static_properties)
            add_static_property(std::move(p));
        for (auto &p : dynamic_properties)
            add_dynamic_property(std::move(p));

        FactBuilder *type_fact_builder = CreateFactBuilder(cc.env, "type");
        FBPutSlotSymbol(type_fact_builder, "id", id.c_str());
        FBPutSlotString(type_fact_builder, "name", name.c_str());
        FBPutSlotString(type_fact_builder, "description", description.c_str());
        type_fact = FBAssert(type_fact_builder);
        FBDispose(type_fact_builder);
        for (auto &p : parents)
            add_parent(p.get());
    }
    type::~type() noexcept
    {
        for (auto &p : parent_facts)
            Retract(p.second);
        Retract(type_fact);
    }

    void type::add_parent(const type &parent) noexcept
    {
        FactBuilder *is_a_fact_builder = CreateFactBuilder(cc.env, "is_a");
        FBPutSlotSymbol(is_a_fact_builder, "type_id", get_id().c_str());
        FBPutSlotSymbol(is_a_fact_builder, "parent_id", parent.get_id().c_str());
        Fact *parent_fact = FBAssert(is_a_fact_builder);
        FBDispose(is_a_fact_builder);
        parents.emplace(parent.name, parent);
        parent_facts.emplace(parent.name, parent_fact);
    }
    void type::remove_parent(const type &parent) noexcept
    {
        Retract(parent_facts.at(parent.name));
        parents.erase(parent.name);
        parent_facts.erase(parent.name);
    }
    void type::add_static_property(std::unique_ptr<property> &&prop) noexcept
    {
        Build(cc.env, prop->to_deftemplate(*this, true).c_str());
        static_properties.emplace(prop->get_name(), std::move(prop));
    }
    void type::remove_static_property(const property &prop) noexcept { static_properties.erase(prop.get_name()); }
    void type::add_dynamic_property(std::unique_ptr<property> &&prop) noexcept
    {
        Build(cc.env, prop->to_deftemplate(*this, true).c_str());
        dynamic_properties.emplace(prop->get_name(), std::move(prop));
    }
    void type::remove_dynamic_property(const property &prop) noexcept { dynamic_properties.erase(prop.get_name()); }

    json::json type::to_json() const noexcept
    {
        json::json j = json::json{{"id", id}, {"name", name}, {"description", description}};
        if (!parents.empty())
        {
            json::json parents_json(json::json_type::array);
            for (const auto &p : parents)
                parents_json.push_back(p.second.get().id);
            j["parents"] = parents_json;
        }
        if (!static_properties.empty())
        {
            json::json static_properties_json(json::json_type::array);
            for (const auto &p : static_properties)
                static_properties_json.push_back(coco::to_json(*p.second));
            j["static_properties"] = static_properties_json;
        }
        if (!dynamic_properties.empty())
        {
            json::json dynamic_properties_json(json::json_type::array);
            for (const auto &p : dynamic_properties)
                dynamic_properties_json.push_back(coco::to_json(*p.second));
            j["dynamic_properties"] = dynamic_properties_json;
        }
        return j;
    }
} // namespace coco