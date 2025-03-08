#include "coco_property.hpp"

namespace coco
{
    property::property(const type &tp, const std::string &name, const std::string &description) noexcept : tp(tp), name(name), description(description) {}
    json::json property::to_json() const noexcept { return description.empty() ? json::json() : json::json{{"description", description.c_str()}}; }

    boolean_property::boolean_property(const type &tp, const std::string &name, const json::json &j) noexcept : property(tp, name, j.contains("description") ? j["description"] : "")
    {
        if (j.contains("default"))
            default_value = j["default"];
    }
    json::json boolean_property::to_json() const noexcept
    {
        json::json j = property::to_json();
        j["type"] = bool_kw;
        if (default_value.has_value())
            j["default"] = default_value.value();
        return j;
    }
} // namespace coco
