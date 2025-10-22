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
    type::type(coco &cc, std::string_view name, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept : cc(cc), name(name), data(std::move(data))
    {
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

    item &type::make_item(std::string_view id, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val)
    {
        auto itm_ptr = std::make_unique<item>(cc, id, std::move(props), std::move(val));
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
