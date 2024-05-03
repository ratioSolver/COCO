#include "sensor_type.hpp"

namespace coco
{
    sensor_type::sensor_type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&pars) : id(id), name(name), description(description), parameters(std::move(pars)) {}
} // namespace coco