#include "coco.hpp"
#include "coco_property.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    coco::coco(coco_db &db) noexcept : db(db), env(CreateEnvironment())
    {
        add_property_type(utils::make_u_ptr<bool_property_type>(*this));

        LOG_TRACE(type_deftemplate);
        Build(env, type_deftemplate);
        LOG_TRACE(is_a_deftemplate);
        Build(env, is_a_deftemplate);
        LOG_TRACE(item_deftemplate);
        Build(env, item_deftemplate);
        LOG_TRACE(instance_of_deftemplate);
        Build(env, instance_of_deftemplate);
        LOG_TRACE(inheritance_rule);
        Build(env, inheritance_rule);
        LOG_TRACE(all_instances_of_function);
        Build(env, all_instances_of_function);
    }
    coco::~coco() { DestroyEnvironment(env); }

    void coco::add_property_type(utils::u_ptr<property_type> pt)
    {
        std::string name = pt->get_name();
        if (!property_types.emplace(name, std::move(pt)).second)
            throw std::invalid_argument("property type `" + name + "` already exists");
    }

    property_type &coco::get_property_type(std::string_view name) const
    {
        if (auto it = property_types.find(name.data()); it != property_types.end())
            return *it->second;
        throw std::out_of_range("property type `" + std::string(name) + "` not found");
    }

    std::string coco::to_string(Fact *f, std::size_t buff_size) const noexcept
    {
        auto *sb = CreateStringBuilder(env, buff_size); // Initial buffer size
        FactPPForm(f, sb, false);
        std::string f_str = sb->contents;
        SBDispose(sb);
        return f_str;
    }
} // namespace coco
