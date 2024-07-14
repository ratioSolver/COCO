#include "coco_type.hpp"

namespace coco
{
    type::type(const std::string &id, const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_properties, std::map<std::string, std::unique_ptr<property>> &&dynamic_properties) noexcept : id(id), name(name), description(description), static_properties(std::move(static_properties)), dynamic_properties(std::move(dynamic_properties)) {}

    json::json type::to_json() const noexcept
    {
        json::json j = json::json{{"id", id}, {"name", name}, {"description", description}};
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