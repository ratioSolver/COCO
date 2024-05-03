#include "sensor.hpp"

namespace coco
{
    sensor::sensor(const std::string &id, const sensor_type &type, const std::string &name, json::json &&data) : id(id), type(type), name(name), data(std::move(data)) {}
} // namespace coco