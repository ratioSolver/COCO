#include "sensor.h"
#include <algorithm>

namespace coco
{
    sensor::sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l) : id(id), name(name), type(type), loc(std::move(l)) {}

    void sensor::add_sensor_listener(sensor_listener &l) { listeners.push_back(&l); }
    void sensor::remove_sensor_listener(sensor_listener &l) { listeners.erase(std::find(listeners.cbegin(), listeners.cend(), &l)); }

    void sensor::set_name(const std::string &n)
    {
        name = n;
        for (auto &l : listeners)
            l->sensor_name_updated(*this, n);
    }

    void sensor::set_location(std::unique_ptr<location> l)
    {
        loc.swap(l);
        for (auto &l : listeners)
            l->sensor_location_updated(*this, *loc);
    }

    void sensor::set_value(const std::chrono::milliseconds::rep &time, const json::json &val)
    {
        last_update = time;
        json::json c_v = json::load(val.dump());
        auto v = std::make_unique<json::json>(std::move(c_v));
        value.swap(v);

        for (auto &l : listeners)
            l->sensor_value_updated(*this, time, *v);
    }
} // namespace coco
