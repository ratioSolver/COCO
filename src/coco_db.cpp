#include "coco_db.hpp"
#include "logging.hpp"
#include <atomic>
#include <iomanip>

namespace coco
{
    coco_db::coco_db(json::json &&config) noexcept : config(std::move(config)) {}

    void coco_db::drop() noexcept {}

    std::vector<db_type> coco_db::get_types() noexcept
    {
        LOG_WARN("Retrieving all the types..");
        return std::vector<db_type>();
    }
    void coco_db::create_type(std::string_view name, const std::vector<std::string> &parents, const json::json &data, const json::json &static_props, const json::json &dynamic_props)
    {
        LOG_WARN(std::string("Creating new type: ") + name.data());
        if (!parents.empty())
        {
            std::string pars_str;
            for (auto p_i = parents.begin(); p_i != parents.end(); ++p_i)
            {
                if (p_i != parents.begin())
                    pars_str += ", ";
                pars_str.append(*p_i);
            }
            LOG_WARN(std::string("Parents: ") + pars_str);
        }
        if (!data.as_object().empty())
            LOG_WARN(std::string("Data: ") + data.dump());
        if (!static_props.as_object().empty())
            LOG_WARN(std::string("Static properties: ") + static_props.dump());
        if (!dynamic_props.as_object().empty())
            LOG_WARN(std::string("Dynamic properties: ") + dynamic_props.dump());
    }
    void coco_db::set_parents(std::string_view name, const std::vector<std::string> &parents)
    {
        if (!parents.empty())
        {
            LOG_WARN(std::string("Setting parents for ") + name.data());
            std::string pars_str;
            for (auto p_i = parents.begin(); p_i != parents.end(); ++p_i)
            {
                if (p_i != parents.begin())
                    pars_str += ", ";
                pars_str.append(*p_i);
            }
            LOG_WARN(std::string("Parents: ") + pars_str);
        }
        else
            LOG_WARN(std::string("Deleting parents for ") + name.data());
    }
    void coco_db::delete_type(std::string_view name)
    {
        LOG_WARN(std::string("Deleting type ") + name.data());
    }

    std::vector<db_item> coco_db::get_items() noexcept
    {
        LOG_WARN("Retrieving all the items..");
        return std::vector<db_item>();
    }
    std::string coco_db::create_item(std::string_view type, const json::json &props, const std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &val)
    {
        static std::atomic<int> counter{0};
        LOG_WARN(std::string("Creating new item of type ") + type.data());
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
} // namespace coco
