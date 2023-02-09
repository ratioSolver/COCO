#include "sensor.h"
#include <algorithm>

namespace coco
{
    sensor::sensor(const std::string &id, const std::string &name, const sensor_type &type, std::unique_ptr<location> l) : id(id), name(name), type(type), loc(std::move(l)) {}

    void sensor::set_value(const std::chrono::milliseconds::rep &time, const json::json &val)
    {
        last_update = time;
        json::json c_v = json::load(val.dump());
        auto v = std::make_unique<json::json>(std::move(c_v));
        value.swap(v);
    }
} // namespace coco
