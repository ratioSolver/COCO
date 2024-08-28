#include "coco_api.hpp"
#include "coco_core.hpp"

namespace coco
{
    [[nodiscard]] json::json to_json(const property &p) noexcept { return p.to_json(); }

    [[nodiscard]] json::json to_json(const type &t) noexcept
    {
        json::json j = json::json{{"id", t.get_id()}, {"name", t.get_name()}, {"description", t.get_description()}};
        if (!t.get_parents().empty())
        {
            json::json parents_json(json::json_type::array);
            for (const auto &p : t.get_parents())
                parents_json.push_back(p.second.get().get_id());
            j["parents"] = parents_json;
        }
        if (!t.get_static_properties().empty())
        {
            json::json static_properties_json;
            for (const auto &p : t.get_static_properties())
                static_properties_json[p.first] = coco::to_json(*p.second);
            j["static_properties"] = std::move(static_properties_json);
        }
        if (!t.get_dynamic_properties().empty())
        {
            json::json dynamic_properties_json;
            for (const auto &p : t.get_dynamic_properties())
                dynamic_properties_json[p.first] = coco::to_json(*p.second);
            j["dynamic_properties"] = std::move(dynamic_properties_json);
        }
        return j;
    }

    [[nodiscard]] json::json to_json(const item &itm) noexcept { return json::json{{"id", itm.get_id()}, {"type", itm.get_type().get_id()}, {"name", itm.get_name()}, {"properties", itm.get_properties()}, {"value", {{"data", itm.get_value()}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(itm.get_timestamp().time_since_epoch()).count()}}}}; }

    [[nodiscard]] json::json to_json(const rule &r) noexcept
    {
        json::json j;
        j["id"] = r.get_id();
        j["name"] = r.get_name();
        j["content"] = r.get_content();
        return j;
    }

    [[nodiscard]] json::json make_types_message(coco_core &core) noexcept
    {
        json::json j;
        j["type"] = "types";
        json::json types = json::json_type::array;
        for (const auto &type : core.get_types())
            types.push_back(to_json(type));
        j["types"] = std::move(types);
        return j;
    }

    [[nodiscard]] json::json make_new_type_message(const type &tp) noexcept
    {
        json::json j;
        j["type"] = "new_type";
        j["new_type"] = to_json(tp);
        return j;
    }

    [[nodiscard]] json::json make_updated_type_message(const type &tp) noexcept
    {
        json::json j;
        j["type"] = "updated_type";
        j["updated_type"] = to_json(tp);
        return j;
    }

    [[nodiscard]] json::json make_deleted_type_message(const std::string &tp_id) noexcept
    {
        json::json j;
        j["type"] = "deleted_type";
        j["deleted_type"] = tp_id;
        return j;
    }

    [[nodiscard]] json::json make_items_message(coco_core &core) noexcept
    {
        json::json j;
        j["type"] = "items";
        json::json items = json::json_type::array;
        for (const auto &item : core.get_items())
            items.push_back(to_json(item));
        j["items"] = std::move(items);
        return j;
    }

    [[nodiscard]] json::json make_new_item_message(const item &itm) noexcept
    {
        json::json j;
        j["type"] = "new_item";
        j["new_item"] = to_json(itm);
        return j;
    }

    [[nodiscard]] json::json make_updated_item_message(const item &itm) noexcept
    {
        json::json j;
        j["type"] = "updated_item";
        j["updated_item"] = to_json(itm);
        return j;
    }

    [[nodiscard]] json::json make_new_value_message(const item &itm) noexcept
    {
        json::json j;
        j["type"] = "new_value";
        j["item_id"] = itm.get_id();
        j["value"] = {{"data", itm.get_value()}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(itm.get_timestamp().time_since_epoch()).count()}};
        return j;
    }

    [[nodiscard]] json::json make_deleted_item_message(const std::string &itm_id) noexcept
    {
        json::json j;
        j["type"] = "deleted_item";
        j["deleted_item"] = itm_id;
        return j;
    }

    [[nodiscard]] json::json make_new_data_message(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) noexcept
    {
        json::json j;
        j["type"] = "new_data";
        j["item_id"] = itm.get_id();
        j["data"] = data;
        j["timestamp"] = std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count();
        return j;
    }

    [[nodiscard]] json::json make_reactive_rules_message(coco_core &core) noexcept
    {
        json::json j;
        j["type"] = "reactive_rules";
        json::json rules = json::json_type::array;
        for (const auto &r : core.get_reactive_rules())
            rules.push_back(to_json(r));
        j["rules"] = std::move(rules);
        return j;
    }

    [[nodiscard]] json::json make_new_reactive_rule_message(const rule &r) noexcept
    {
        json::json j = to_json(r);
        j["type"] = "new_reactive_rule";
        return j;
    }

    [[nodiscard]] json::json make_updated_reactive_rule_message(const rule &r) noexcept
    {
        json::json j = to_json(r);
        j["type"] = "updated_reactive_rule";
        return j;
    }

    [[nodiscard]] json::json make_deleted_reactive_rule_message(const std::string &r_id) noexcept
    {
        json::json j;
        j["type"] = "deleted_reactive_rule";
        j["deleted_reactive_rule"] = r_id;
        return j;
    }

    [[nodiscard]] json::json make_deliberative_rules_message(coco_core &core) noexcept
    {
        json::json j;
        j["type"] = "deliberative_rules";
        json::json rules = json::json_type::array;
        for (const auto &r : core.get_deliberative_rules())
            rules.push_back(to_json(r));
        j["rules"] = std::move(rules);
        return j;
    }

    [[nodiscard]] json::json make_new_deliberative_rule_message(const rule &r) noexcept
    {
        json::json j = to_json(r);
        j["type"] = "new_deliberative_rule";
        return j;
    }

    [[nodiscard]] json::json make_updated_deliberative_rule_message(const rule &r) noexcept
    {
        json::json j = to_json(r);
        j["type"] = "updated_deliberative_rule";
        return j;
    }

    [[nodiscard]] json::json make_deleted_deliberative_rule_message(const std::string &r_id) noexcept
    {
        json::json j;
        j["type"] = "deleted_deliberative_rule";
        j["deleted_deliberative_rule"] = r_id;
        return j;
    }

    [[nodiscard]] json::json make_solvers_message(coco_core &core) noexcept
    {
        json::json j;
        j["type"] = "solvers";
        json::json solvers = json::json_type::array;
        for (const auto &exec : core.get_solvers())
            solvers.push_back(to_json(exec));
        j["solvers"] = std::move(solvers);
        return j;
    }
} // namespace coco
