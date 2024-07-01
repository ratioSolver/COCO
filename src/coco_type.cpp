#include "coco_type.hpp"

namespace coco
{
    type::type(const std::string &id, const std::string &name, const std::string &description, std::map<std::string, std::reference_wrapper<parameter>> &&static_pars, std::map<std::string, std::reference_wrapper<parameter>> &&dynamic_pars) : id(id), name(name), description(description), static_parameters(std::move(static_pars)), dynamic_parameters(std::move(dynamic_pars)) {}
} // namespace coco