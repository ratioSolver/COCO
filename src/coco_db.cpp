#include "coco_db.h"
#include <functional>
#include <cassert>

namespace coco
{
    coco_db::coco_db(const std::string &root) : root(root) {}

    void coco_db::init()
    {
        sensor_types.clear();
        sensors.clear();
    }

    std::string coco_db::create_user(const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &roots, const json::json &data)
    {
        size_t c_id = users.size();
        while (users.count(std::to_string(c_id)))
            c_id++;
        create_user(std::to_string(c_id), first_name, last_name, email, password, roots, data);
        return std::to_string(c_id);
    }
    user &coco_db::create_user(const std::string &id, const std::string &first_name, const std::string &last_name, const std::string &email, const std::string &password, const std::vector<std::string> &roots, const json::json &data)
    {
        auto u = new user(id, first_name, last_name, email, password, roots, data);
        if (std::find_if(roots.begin(), roots.end(), [&](const std::string &r)
                         { return r == root; }) != roots.end())
            users[id] = u;
        return *u;
    }
    std::vector<std::reference_wrapper<user>> coco_db::get_users()
    {
        std::vector<std::reference_wrapper<user>> us;
        us.reserve(users.size());
        for (auto &[id, u] : users)
            us.push_back(*u);
        return us;
    }
    void coco_db::set_user_roots(user &u, const std::vector<std::string> &roots)
    {
        u.roots = roots;
        auto it = std::find_if(roots.begin(), roots.end(), [&](const std::string &r)
                               { return r == root; });
        if (it != roots.end())
            users[u.id] = &u;
        else
            users.erase(u.id);
    }

    std::string coco_db::create_sensor_type(const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        size_t c_id = sensor_types.size();
        while (sensor_types.count(std::to_string(c_id)))
            c_id++;
        create_sensor_type(std::to_string(c_id), name, description, parameter_types);
        return std::to_string(c_id);
    }
    sensor_type &coco_db::create_sensor_type(const std::string &id, const std::string &name, const std::string &description, const std::map<std::string, parameter_type> &parameter_types)
    {
        auto st = new sensor_type(id, name, description, parameter_types);
        sensor_types[id] = st;
        sensor_types_by_name.emplace(name, *st);
        return *st;
    }
    std::vector<std::reference_wrapper<sensor_type>> coco_db::get_sensor_types()
    {
        std::vector<std::reference_wrapper<sensor_type>> sts;
        sts.reserve(sensor_types.size());
        for (auto &[id, st] : sensor_types)
            sts.push_back(*st);
        return sts;
    }

    std::string coco_db::create_sensor(const std::string &name, sensor_type &type, location_ptr l)
    {
        size_t c_id = sensors.size();
        while (sensors.count(std::to_string(c_id)))
            c_id++;
        create_sensor(std::to_string(c_id), name, type, std::move(l));
        return std::to_string(c_id);
    }
    sensor &coco_db::create_sensor(const std::string &id, const std::string &name, sensor_type &type, location_ptr l)
    {
        auto s = new sensor(id, name, type, std::move(l));
        type.sensors.push_back(*s);
        sensors[id] = s;
        assert(&s->type == &type);
        return *s;
    }
    std::vector<std::reference_wrapper<sensor>> coco_db::get_sensors()
    {
        std::vector<std::reference_wrapper<sensor>> sts;
        sts.reserve(sensors.size());
        for (auto &[id, st] : sensors)
            sts.push_back(*st);
        return sts;
    }
    json::json coco_db::get_last_sensor_value([[maybe_unused]] sensor &s) { return s.has_value() ? s.get_value() : json::json(); }
    json::json coco_db::get_sensor_values([[maybe_unused]] sensor &s, [[maybe_unused]] const std::chrono::system_clock::time_point &from, [[maybe_unused]] const std::chrono::system_clock::time_point &to) { return json::json(json::json_type::array); }
    void coco_db::delete_sensor(sensor &s)
    {
        s.type.sensors.erase(std::find_if(s.type.sensors.begin(), s.type.sensors.end(), [&](auto &s2)
                                          { return s2.get().id == s.id; }));
        sensors.erase(s.id);
    }

    void coco_db::drop()
    {
        sensors.clear();
        sensor_types.clear();
    }
} // namespace coco
