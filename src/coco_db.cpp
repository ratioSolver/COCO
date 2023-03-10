#include "coco_db.h"
#include <functional>

namespace coco
{
    coco_db::coco_db(const std::string &root) : root(root) {}

    void coco_db::init()
    {
        sensor_types.clear();
        sensors.clear();
    }

    std::string coco_db::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        size_t c_id = sensor_types.size();
        while (sensor_types.count(std::to_string(c_id)))
            c_id++;
        create_sensor_type(std::to_string(c_id), name, description, parameter_types);
        return std::to_string(c_id);
    }
    void coco_db::create_sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types) { sensor_types[id] = new sensor_type(id, name, description, parameter_types); }
    std::vector<std::reference_wrapper<sensor_type>> coco_db::get_all_sensor_types()
    {
        std::vector<std::reference_wrapper<sensor_type>> sts;
        sts.reserve(sensor_types.size());
        for (auto &[id, st] : sensor_types)
            sts.push_back(*st);
        return sts;
    }
    void coco_db::set_sensor_type_name(sensor_type &st, const std::string &name) { st.name = name; }
    void coco_db::set_sensor_type_description(sensor_type &st, const std::string &description) { st.description = description; }
    void coco_db::delete_sensor_type(sensor_type &st) { sensor_types.erase(st.id); }

    std::string coco_db::create_sensor(const std::string &name, const sensor_type &type, location_ptr l)
    {
        size_t c_id = sensors.size();
        while (sensors.count(std::to_string(c_id)))
            c_id++;
        create_sensor(std::to_string(c_id), name, type, std::move(l));
        return std::to_string(c_id);
    }
    void coco_db::create_sensor(const std::string &id, const std::string &name, const sensor_type &type, location_ptr l) { sensors[id] = new sensor(id, name, type, std::move(l)); }
    std::vector<std::reference_wrapper<sensor>> coco_db::get_all_sensors()
    {
        std::vector<std::reference_wrapper<sensor>> sts;
        sts.reserve(sensors.size());
        for (auto &[id, st] : sensors)
            sts.push_back(*st);
        return sts;
    }
    json::json coco_db::get_sensor_values([[maybe_unused]] sensor &s, [[maybe_unused]] const std::chrono::milliseconds::rep &from, [[maybe_unused]] const std::chrono::milliseconds::rep &to) { return json::json(json::json_type::array); }
    void coco_db::delete_sensor(sensor &s) { sensors.erase(s.id); }

    std::string coco_db::create_user(const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const json::json &data)
    {
        size_t c_id = users.size();
        while (users.count(std::to_string(c_id)))
            c_id++;
        create_user(std::to_string(c_id), first_name, last_name, email, password, data);
        return std::to_string(c_id);
    }
    void coco_db::create_user(const std::string &id, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const json::json &data) { users[id] = new user(id, first_name, last_name, email, password, data); }
    std::vector<std::reference_wrapper<user>> coco_db::get_all_users()
    {
        std::vector<std::reference_wrapper<user>> sts;
        sts.reserve(users.size());
        for (auto &[id, st] : users)
            sts.push_back(*st);
        return sts;
    }
    void coco_db::delete_user(user &u) { users.erase(u.id); }

    void coco_db::drop()
    {
        sensors.clear();
        sensor_types.clear();
    }
} // namespace coco
