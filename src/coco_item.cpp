#include "coco_item.hpp"

namespace coco
{
    item::item(const std::string &id, const type &tp, const std::string &name, const json::json &props) : id(id), tp(tp), name(name), properties(props) {}
} // namespace coco