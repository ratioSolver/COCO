#include "coco_item.hpp"

namespace coco
{
    item::item(const std::string &id, const type &tp, const std::string &name, json::json &&data) : id(id), tp(tp), name(name), data(std::move(data)) {}
} // namespace coco