#include "coco_item.hpp"

namespace coco
{
    item::item(const std::string &id, const type &tp, const std::string &name, const json::json &pars) : id(id), tp(tp), name(name), parameters(pars) {}
} // namespace coco