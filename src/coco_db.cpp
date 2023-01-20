#include "coco_db.h"

namespace coco
{
    coco_db::coco_db(const std::string &root) : root(root) {}

    std::string coco_db::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        size_t c_id = sensor_types.size();
        while (sensor_types.count(std::to_string(c_id)))
            c_id++;
        create_sensor_type(std::to_string(c_id), name, description, parameter_types);
        return std::to_string(c_id);
    }
    void coco_db::create_sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) { sensor_types[id] = std::make_unique<sensor_type>(id, name, description, parameter_types); }
    std::vector<std::reference_wrapper<sensor_type>> coco_db::get_all_sensor_types()
    {
        std::vector<std::reference_wrapper<sensor_type>> sts;
        sts.reserve(sensor_types.size());
        for (auto &[id, st] : sensor_types)
            sts.push_back(*st);
        return sts;
    }
    void coco_db::set_sensor_type_name(const std::string &id, const std::string &name) { sensor_types.at(id)->name = name; }
    void coco_db::set_sensor_type_description(const std::string &id, const std::string &description) { sensor_types.at(id)->description = description; }
    void coco_db::delete_sensor_type(const std::string &id) { sensor_types.erase(id); }

    std::string coco_db::create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l)
    {
        size_t c_id = sensors.size();
        while (sensors.count(std::to_string(c_id)))
            c_id++;
        create_sensor(std::to_string(c_id), name, type, std::move(l));
        return std::to_string(c_id);
    }
    void coco_db::create_sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l) { sensors[id] = std::make_unique<sensor>(id, name, type, std::move(l)); }
    std::vector<std::reference_wrapper<sensor>> coco_db::get_all_sensors()
    {
        std::vector<std::reference_wrapper<sensor>> sts;
        sts.reserve(sensors.size());
        for (auto &[id, st] : sensors)
            sts.push_back(*st);
        return sts;
    }
    void coco_db::set_sensor_name(const std::string &id, const std::string &name) { sensors.at(id)->name = name; }
    void coco_db::set_sensor_location(const std::string &id, std::unique_ptr<location> l) { sensors.at(id)->loc.swap(l); }
    json::json coco_db::get_sensor_values([[maybe_unused]] const std::string &id, [[maybe_unused]] const std::chrono::milliseconds::rep &from, [[maybe_unused]] const std::chrono::milliseconds::rep &to) { return json::json(); }
    void coco_db::set_sensor_value(const std::string &id, const std::chrono::milliseconds::rep &time, const json::json &val)
    {
        json::json c_v = json::load(val.dump());
        auto v = std::make_unique<json::json>(std::move(c_v));
        sensors.at(id)->last_update = time;
        sensors.at(id)->value.swap(v);
    }
    void coco_db::delete_sensor(const std::string &id) { sensors.erase(id); }

    void coco_db::drop()
    {
        sensors.clear();
        sensor_types.clear();
    }
} // namespace coco
