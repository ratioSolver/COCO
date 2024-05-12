#include "coco_type.hpp"

namespace coco
{
    type::type(const std::string &id, const std::string &name, const std::string &description, std::vector<std::unique_ptr<parameter>> &&static_pars, std::vector<std::unique_ptr<parameter>> &&dynamic_pars) : id(id), name(name), description(description), static_parameters(std::move(static_pars)), dynamic_parameters(std::move(dynamic_pars)) {}
} // namespace coco