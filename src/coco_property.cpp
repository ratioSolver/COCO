#include "coco_property.hpp"

namespace coco
{
    property::property(const type &tp, const std::string &name, const std::string &description) noexcept : tp(tp), name(name), description(description) {}
} // namespace coco
