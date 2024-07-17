#include "coco_type.hpp"

namespace coco
{
    type::type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties) noexcept : id(id), name(name), description(description)
    {
        for (auto &p : parents)
            this->parents.emplace(p.get().name, p);
        for (auto &p : static_properties)
            this->static_properties.emplace(p->get_name(), std::move(p));
        for (auto &p : dynamic_properties)
            this->dynamic_properties.emplace(p->get_name(), std::move(p));
    }

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