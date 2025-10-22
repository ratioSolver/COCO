#include "coco_db.hpp"
#include "logging.hpp"
#include <atomic>
#include <iomanip>

namespace coco
{
    db_module::db_module(coco_db &db) noexcept : db(db) {}
    void db_module::drop() noexcept {}

    coco_db::coco_db(json::json &&config) noexcept : config(std::move(config)) {}

    void coco_db::drop() noexcept
    {
        LOG_WARN("Dropping database..");
        for (auto &[_, mod] : modules)
            mod->drop();
    }

    std::vector<db_type> coco_db::get_types() noexcept
    {
        LOG_WARN("Retrieving all the types..");
        return std::vector<db_type>();
    }
    void coco_db::create_type(std::string_view tp_name, const json::json &data, const json::json &static_props, const json::json &dynamic_props)
    {
        LOG_WARN(std::string("Creating new type: ") + tp_name.data());
        if (!data.as_object().empty())
            LOG_WARN(std::string("Data: ") + data.dump());
        if (!static_props.as_object().empty())
            LOG_WARN(std::string("Static properties: ") + static_props.dump());
        if (!dynamic_props.as_object().empty())
            LOG_WARN(std::string("Dynamic properties: ") + dynamic_props.dump());
    }
    void coco_db::delete_type(std::string_view name) { LOG_WARN(std::string("Deleting type ") + name.data()); }

    std::vector<db_item> coco_db::get_items() noexcept
    {
        LOG_WARN("Retrieving all the items..");
        return std::vector<db_item>();
    }
    std::string coco_db::create_item(std::string_view tp_name, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val)
    {
        static std::atomic<int> counter{0};
        LOG_WARN(std::string("Creating new item of type ") + tp_name.data());
        if (!props.as_object().empty())
            LOG_WARN(std::string("Properties: ") + props.dump());
        if (val.has_value())
        {
            LOG_WARN(std::string("Value: ") + val->first.dump());
            std::time_t time = std::chrono::system_clock::to_time_t(val->second);
            std::tm tm = *std::localtime(&time);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
            LOG_WARN(std::string("Timestamp: ") + oss.str());
        }
        return std::to_string(counter++);
    }
    void coco_db::set_properties(std::string_view itm_id, const json::json &props)
    {
        LOG_WARN(std::string("Setting properties for item ") + itm_id.data());
        if (!props.as_object().empty())
            LOG_WARN(std::string("Properties: ") + props.dump());
    }
    json::json coco_db::get_values(std::string_view itm_id, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        LOG_WARN(std::string("Getting values for item ") + itm_id.data());
        std::time_t from_time = std::chrono::system_clock::to_time_t(from);
        std::tm from_tm = *std::localtime(&from_time);
        std::ostringstream from_oss;
        from_oss << std::put_time(&from_tm, "%Y-%m-%d %H:%M:%S");
        LOG_WARN(std::string("FROM: ") + from_oss.str());
        std::time_t to_time = std::chrono::system_clock::to_time_t(to);
        std::tm to_tm = *std::localtime(&to_time);
        std::ostringstream to_oss;
        to_oss << std::put_time(&to_tm, "%Y-%m-%d %H:%M:%S");
        LOG_WARN(std::string("FROM: ") + to_oss.str());
        json::json res(json::json_type::array);
        return res;
    }
    void coco_db::set_value(std::string_view itm_id, const json::json &val, const std::chrono::system_clock::time_point &timestamp)
    {
        LOG_WARN(std::string("Setting value for item ") + itm_id.data());
        LOG_WARN(std::string("Value: ") + val.dump());
        std::time_t time = std::chrono::system_clock::to_time_t(timestamp);
        std::tm tm = *std::localtime(&time);
        std::ostringstream oss;
        oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        LOG_WARN(std::string("Timestamp: ") + oss.str());
    }
    void coco_db::delete_item(std::string_view itm_id) { LOG_WARN(std::string("Deleting item ") + itm_id.data()); }

    std::vector<db_rule> coco_db::get_reactive_rules() noexcept
    {
        LOG_WARN("Retrieving all the reactive rules..");
        return std::vector<db_rule>();
    }
    void coco_db::create_reactive_rule(std::string_view rule_name, std::string_view rule_content)
    {
        LOG_WARN(std::string("Creating new reactive rule: ") + rule_name.data());
        LOG_WARN(std::string("Content: ") + rule_content.data());
    }
} // namespace coco
