#include "coco_item.hpp"

namespace coco
{
    item::item(const std::string &id, const type &tp, const std::string &name, std::vector<std::unique_ptr<parameter>> &&static_pars) : id(id), tp(tp), name(name), parameters(std::move(static_pars)) {}
} // namespace coco