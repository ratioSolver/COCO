#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_db.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

#ifdef BUILD_LISTENERS
#define NEW_TYPE(tp) new_type(tp)
#else
#define NEW_TYPE(tp)
#endif

namespace coco
{
    coco::coco(coco_db &db) noexcept : db(db), env(CreateEnvironment())
    {
        add_property_type(utils::make_u_ptr<bool_property_type>(*this));
        add_property_type(utils::make_u_ptr<int_property_type>(*this));
        add_property_type(utils::make_u_ptr<float_property_type>(*this));

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

        LOG_DEBUG("Retrieving all types");
        auto tps = db.get_types();
        LOG_DEBUG("Retrieved " << tps.size() << " types");
        for (auto &tp : tps)
            make_type(tp.name, {}, tp.static_props.has_value() ? std::move(tp.static_props.value()) : json::json{}, tp.static_props.has_value() ? std::move(tp.static_props.value()) : json::json{}, tp.static_props.has_value() ? std::move(tp.static_props.value()) : json::json{});

        for (auto &tp : tps)
            if (!tp.parents.empty())
            {
                std::vector<utils::ref_wrapper<const type>> parents;
                for (auto &parent : tp.parents)
                    parents.emplace_back(get_type(parent));
                get_type(tp.name).set_parents(std::move(parents));
            }

        LOG_DEBUG("Retrieving all items");
        for (auto &itm : db.get_items())
            get_type(itm.type).make_item(itm.id, itm.props.has_value() ? std::move(itm.props.value()) : json::json{});

        Run(env, -1);
    }
    coco::~coco()
    {
        types.clear();
        DestroyEnvironment(env);
    }

    std::vector<utils::ref_wrapper<type>> coco::get_types() noexcept
    {
        std::vector<utils::ref_wrapper<type>> res;
        for (auto &s : types)
            res.push_back(*s.second);
        return res;
    }

    type &coco::get_type(const std::string &name)
    {
        if (types.find(name) == types.end())
            throw std::invalid_argument("Type not found: " + name);
        return *types.at(name);
    }

    type &coco::create_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept
    {
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent->get_name());
        db.create_type(name, parents_names, data, static_props, dynamic_props);
        return make_type(name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
    }
    void coco::set_parents(type &tp, std::vector<utils::ref_wrapper<const type>> &&parents) noexcept
    {
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent->get_name());
        db.set_parents(tp.get_name(), parents_names);
        tp.set_parents(std::move(parents));
    }
    void coco::delete_type(type &tp) noexcept
    {
        types.erase(tp.get_name());
        db.delete_type(tp.get_name());
    }

    item &coco::create_item(type &tp, json::json &&props, json::json &&val, const std::chrono::system_clock::time_point &timestamp) noexcept
    {
        auto id = db.create_item(tp.get_name(), props, val, timestamp);
        return tp.make_item(id, std::move(props), std::move(val), timestamp);
    }

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

    type &coco::make_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data) noexcept
    {
        auto tp_ptr = utils::make_u_ptr<type>(*this, name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
        auto &tp = *tp_ptr;
        types.emplace(name, std::move(tp_ptr));
        NEW_TYPE(tp);
        return tp;
    }

#ifdef BUILD_LISTENERS
    void coco::new_type(const type &tp)
    {
        for (auto &l : listeners)
            l->new_type(tp);
    }

    void coco::new_item(const item &itm)
    {
        for (auto &l : listeners)
            l->new_item(itm);
    }

    listener::listener(coco &cc) noexcept : cc(cc) { cc.listeners.emplace_back(this); }
    listener::~listener() { cc.listeners.erase(std::remove(cc.listeners.begin(), cc.listeners.end(), this), cc.listeners.end()); }
#endif

    std::string coco::to_string(Fact *f, std::size_t buff_size) const noexcept
    {
        auto *sb = CreateStringBuilder(env, buff_size); // Initial buffer size
        FactPPForm(f, sb, false);
        std::string f_str = sb->contents;
        SBDispose(sb);
        return f_str;
    }
} // namespace coco
