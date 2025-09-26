#include "coco.hpp"
#include "coco_module.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "coco_db.hpp"
#include "logging.hpp"
#include <algorithm>
#include <functional>
#include <cassert>

namespace coco
{
    coco::coco(coco_db &db) noexcept : db(db), env(CreateEnvironment())
    {
        add_property_type(std::make_unique<bool_property_type>(*this));
        add_property_type(std::make_unique<int_property_type>(*this));
        add_property_type(std::make_unique<float_property_type>(*this));
        add_property_type(std::make_unique<string_property_type>(*this));
        add_property_type(std::make_unique<symbol_property_type>(*this));
        add_property_type(std::make_unique<item_property_type>(*this));
        add_property_type(std::make_unique<json_property_type>(*this));

        [[maybe_unused]] auto set_props_err = AddUDF(env, "set_properties", "v", 3, 3, "ymm", set_props, "set_props", this);
        assert(set_props_err == AUE_NO_ERROR);
        [[maybe_unused]] auto add_data_err = AddUDF(env, "add_data", "v", 3, 4, "ymml", add_data, "add_data", this);
        assert(add_data_err == AUE_NO_ERROR);
        [[maybe_unused]] auto to_json_err = AddUDF(env, "to_json", "s", 1, 1, "m", multifield_to_json, "multifield_to_json", this);
        assert(to_json_err == AUE_NO_ERROR);

        LOG_TRACE(type_deftemplate);
        [[maybe_unused]] auto build_type_dt_err = Build(env, type_deftemplate);
        assert(build_type_dt_err == BE_NO_ERROR);
        LOG_TRACE(is_a_deftemplate);
        [[maybe_unused]] auto build_is_a_dt_err = Build(env, is_a_deftemplate);
        assert(build_is_a_dt_err == BE_NO_ERROR);
        LOG_TRACE(instance_of_deftemplate);
        [[maybe_unused]] auto build_instance_dt_err = Build(env, instance_of_deftemplate);
        assert(build_instance_dt_err == BE_NO_ERROR);
        LOG_TRACE(inheritance_rule);
        [[maybe_unused]] auto build_inh_rl_err = Build(env, inheritance_rule);
        assert(build_inh_rl_err == BE_NO_ERROR);
        LOG_TRACE(all_instances_of_function);
        [[maybe_unused]] auto build_all_insts_fn_err = Build(env, all_instances_of_function);
        assert(build_all_insts_fn_err == BE_NO_ERROR);

        LOG_DEBUG("Retrieving all types");
        auto tps = db.get_types();
        LOG_DEBUG("Retrieved " << tps.size() << " types");

        std::map<std::string, db_type> tps_map;
        for (const auto &tp : tps)
            tps_map[tp.name] = tp;

        std::function<void(const std::string &)> visit = [&](const std::string &name)
        {
            if (types.count(name))
                return;
            for (const auto &parent : tps_map.at(name).parents)
                visit(parent);
            std::vector<std::reference_wrapper<const type>> parents;
            for (auto &parent : tps_map.at(name).parents)
                parents.emplace_back(get_type(parent));
            make_type(name, std::move(parents), tps_map.at(name).static_props.has_value() ? std::move(*tps_map.at(name).static_props) : json::json{}, tps_map.at(name).dynamic_props.has_value() ? std::move(*tps_map.at(name).dynamic_props) : json::json{}, tps_map.at(name).data.has_value() ? std::move(*tps_map.at(name).data) : json::json{});
        };

        for (const auto &tp : tps)
            visit(tp.name);

        LOG_DEBUG("Retrieving all items");
        auto itms = db.get_items();
        LOG_DEBUG("Retrieved " << itms.size() << " items");
        for (auto &itm : itms)
            get_type(itm.type).make_item(itm.id, itm.props.has_value() ? std::move(itm.props.value()) : json::json{}, itm.value.has_value() ? std::make_optional(std::move(itm.value.value())) : std::nullopt);
    }
    coco::~coco()
    {
        items.clear();
        reactive_rules.clear();
        types.clear();
        Reset(env);
        [[maybe_unused]] auto ce = Clear(env);
        assert(ce);
        [[maybe_unused]] auto de = DestroyEnvironment(env);
        assert(de);
    }

    void coco::load_rules() noexcept
    {
        LOG_DEBUG("Retrieving all reactive rules");
        auto rrs = db.get_reactive_rules();
        LOG_DEBUG("Retrieved " << rrs.size() << " reactive rules");
        for (auto &rule : rrs)
            reactive_rules.emplace(rule.name, std::make_unique<reactive_rule>(*this, rule.name, rule.content));

        Run(env, -1);
    }

    std::vector<std::reference_wrapper<type>> coco::get_types() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<type>> res;
        res.reserve(types.size());
        for (auto &t : types)
            res.push_back(*t.second);
        return res;
    }

    type &coco::get_type(std::string_view name)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        if (types.find(name) == types.end())
            throw std::invalid_argument("Type not found: " + std::string(name));
        return *types.at(name.data());
    }

    type &coco::create_type(std::string_view name, std::vector<std::reference_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent.get().get_name());
        db.create_type(name, parents_names, data, static_props, dynamic_props);
        auto &tp = make_type(name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
        if (infere)
            Run(env, -1);
        return tp;
    }
    void coco::set_parents(type &tp, std::vector<std::reference_wrapper<const type>> &&parents, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent.get().get_name());
        db.set_parents(tp.get_name(), parents_names);
        tp.set_parents(std::move(parents));
        if (infere)
            Run(env, -1);
    }

    void coco::delete_type(type &tp, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.delete_type(tp.get_name());
        types.erase(tp.get_name());
        if (infere)
            Run(env, -1);
    }

    std::vector<std::reference_wrapper<item>> coco::get_items() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<item>> res;
        res.reserve(items.size());
        for (auto &i : items)
            res.push_back(*i.second);
        return res;
    }

    std::vector<std::reference_wrapper<item>> coco::get_items(const type &tp) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<item>> res;
        res.reserve(tp.get_instances().size());
        for (auto &i : tp.get_instances())
            res.push_back(i);
        return res;
    }

    item &coco::get_item(std::string_view id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        if (items.find(id.data()) == items.end())
            throw std::invalid_argument("Item not found: " + std::string(id));
        return *items.at(id.data());
    }
    item &coco::create_item(type &tp, json::json &&props, std::optional<std::pair<json::json, std::chrono::system_clock::time_point>> &&val, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = db.create_item(tp.get_name(), props, val);
        auto &itm = tp.make_item(id, std::move(props), std::move(val));
        if (infere)
            Run(env, -1);
        return itm;
    }
    void coco::set_properties(item &itm, json::json &&props, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.set_properties(itm.get_id(), props);
        itm.set_properties(std::move(props));
        if (infere)
            Run(env, -1);
    }
    json::json coco::get_values(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db.get_values(itm.get_id(), from, to);
    }
    void coco::set_value(item &itm, json::json &&val, const std::chrono::system_clock::time_point &timestamp, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.set_value(itm.get_id(), val, timestamp);
        itm.set_value(std::make_pair(std::move(val), timestamp));
        if (infere)
            Run(env, -1);
    }
    void coco::delete_item(item &itm, bool infere) noexcept
    {
        auto id = itm.get_id();
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.delete_item(id);
        items.erase(id);
        if (infere)
            Run(env, -1);
    }

    std::vector<std::reference_wrapper<reactive_rule>> coco::get_reactive_rules() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<reactive_rule>> res;
        res.reserve(reactive_rules.size());
        for (auto &r : reactive_rules)
            res.push_back(*r.second);
        return res;
    }
    void coco::create_reactive_rule(std::string_view rule_name, std::string_view rule_content, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.create_reactive_rule(rule_name, rule_content);
        auto it = reactive_rules.emplace(rule_name, std::make_unique<reactive_rule>(*this, rule_name, rule_content));
        if (!it.second)
            throw std::invalid_argument("reactive rule `" + std::string(rule_name) + "` already exists");
        else
            CREATED_REACTIVE_RULE(*it.first->second);
        if (infere)
            Run(env, -1);
    }

    void coco::add_property_type(std::unique_ptr<property_type> pt)
    {
        std::string_view name = pt->get_name();
        if (!property_types.emplace(name, std::move(pt)).second)
            throw std::invalid_argument("property type `" + std::string(name) + "` already exists");
    }

    property_type &coco::get_property_type(std::string_view name) const
    {
        if (auto it = property_types.find(name); it != property_types.end())
            return *it->second;
        throw std::out_of_range("property type `" + std::string(name) + "` not found");
    }

    type &coco::make_type(std::string_view name, std::vector<std::reference_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data)
    {
        auto tp_ptr = std::make_unique<type>(*this, name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
        auto &tp = *tp_ptr;
        if (!types.emplace(name, std::move(tp_ptr)).second)
            throw std::invalid_argument("type `" + std::string(name) + "` already exists");
        return tp;
    }

    reactive_rule::reactive_rule(coco &cc, std::string_view name, std::string_view content) noexcept : cc(cc), name(name), content(content)
    {
        LOG_TRACE(content);
        [[maybe_unused]] auto build_rl_err = Build(cc.env, content.data());
        assert(build_rl_err == BE_NO_ERROR);
    }
    reactive_rule::~reactive_rule()
    {
        auto defrule = FindDefrule(cc.env, name.c_str());
        assert(defrule);
        assert(DefruleIsDeletable(defrule));
        [[maybe_unused]] auto undef_rl = Undefrule(defrule, cc.env);
        assert(undef_rl);
    }

    json::json reactive_rule::to_json() const noexcept { return {{"content", content}}; }

    json::json coco::to_json() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        json::json jc;
        if (!types.empty())
        {
            json::json jtps;
            for (auto &[name, tp] : types)
                jtps[name] = tp->to_json();
            jc["types"] = std::move(jtps);
        }
        if (!items.empty())
        {
            json::json jitms;
            for (auto &[id, itm] : items)
                jitms[id] = itm->to_json();
            jc["items"] = std::move(jitms);
        }
        if (!reactive_rules.empty())
        {
            json::json jrrs;
            for (auto &[name, rr] : reactive_rules)
                jrrs[name] = rr->to_json();
            jc["reactive_rules"] = std::move(jrrs);
        }
        for (auto &[_, mdl] : modules)
            mdl->to_json(jc);
        return jc;
    }

    void set_props(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Setting properties..");

        auto &cc = *reinterpret_cast<coco *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        auto &itm = *cc.items.at(item_id.lexemeValue->contents);
        std::map<std::string, std::reference_wrapper<const property>> static_props = itm.get_type().get_all_static_properties();

        UDFValue pars; // we get the parameters..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &pars))
            return;

        UDFValue vals; // we get the values..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &vals))
            return;

        json::json data;
        for (size_t i = 0; i < pars.multifieldValue->length; ++i)
        {
            auto &par = pars.multifieldValue->contents[i];
            if (par.header->type != SYMBOL_TYPE)
                return;
            auto &val = vals.multifieldValue->contents[i];
            switch (val.header->type)
            {
            case INTEGER_TYPE:
                data[par.lexemeValue->contents] = static_cast<int64_t>(val.integerValue->contents);
                break;
            case FLOAT_TYPE:
                data[par.lexemeValue->contents] = val.floatValue->contents;
                break;
            case STRING_TYPE:
                data[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            case SYMBOL_TYPE:
                if (std::string(val.lexemeValue->contents) == "TRUE")
                    data[par.lexemeValue->contents] = true;
                else if (std::string(val.lexemeValue->contents) == "FALSE")
                    data[par.lexemeValue->contents] = false;
                else if (std::string(val.lexemeValue->contents) == "nil")
                    data[par.lexemeValue->contents] = nullptr;
                else
                    data[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            default:
                return;
            }
        }

        cc.set_properties(itm, std::move(data), false);
    }

    void add_data(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Adding data..");

        auto &cc = *reinterpret_cast<coco *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        auto &itm = *cc.items.at(item_id.lexemeValue->contents);
        std::map<std::string, std::reference_wrapper<const property>> dynamic_props = itm.get_type().get_all_dynamic_properties();

        UDFValue pars; // we get the parameters..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &pars))
            return;

        UDFValue vals; // we get the values..
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &vals))
            return;

        json::json data;
        for (size_t i = 0; i < pars.multifieldValue->length; ++i)
        {
            auto &par = pars.multifieldValue->contents[i];
            if (par.header->type != SYMBOL_TYPE)
                return;
            auto &val = vals.multifieldValue->contents[i];
            switch (val.header->type)
            {
            case INTEGER_TYPE:
                data[par.lexemeValue->contents] = static_cast<int64_t>(val.integerValue->contents);
                break;
            case FLOAT_TYPE:
                data[par.lexemeValue->contents] = val.floatValue->contents;
                break;
            case STRING_TYPE:
                if (dynamic_props.at(par.lexemeValue->contents).get().is_complex())
                    data[par.lexemeValue->contents] = json::load(val.lexemeValue->contents);
                else
                    data[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            case SYMBOL_TYPE:
                if (std::string(val.lexemeValue->contents) == "TRUE")
                    data[par.lexemeValue->contents] = true;
                else if (std::string(val.lexemeValue->contents) == "FALSE")
                    data[par.lexemeValue->contents] = false;
                else if (std::string(val.lexemeValue->contents) == "nil")
                    data[par.lexemeValue->contents] = nullptr;
                else
                    data[par.lexemeValue->contents] = val.lexemeValue->contents;
                break;
            default:
                return;
            }
        }

        UDFValue timestamp; // we get the timestamp..
        if (UDFHasNextArgument(udfc))
        {
            if (!UDFNextArgument(udfc, INTEGER_BIT, &timestamp))
                return;
            cc.set_value(itm, std::move(data), std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp.integerValue->contents)), false);
        }
        else
            cc.set_value(itm, std::move(data), std::chrono::system_clock::now(), false);
    }

    void multifield_to_json(Environment *env, UDFContext *udfc, UDFValue *ret)
    {
        UDFValue multifield;
        if (!UDFFirstArgument(udfc, MULTIFIELD_BIT, &multifield))
            return;

        json::json j_array(json::json_type::array);

        for (size_t i = 0; i < multifield.multifieldValue->length; ++i)
        {
            auto &val = multifield.multifieldValue->contents[i];
            switch (val.header->type)
            {
            case STRING_TYPE:
            case SYMBOL_TYPE:
                j_array.push_back(val.lexemeValue->contents);
                break;
            case INTEGER_TYPE:
                j_array.push_back(static_cast<int64_t>(val.integerValue->contents));
                break;
            case FLOAT_TYPE:
                j_array.push_back(val.floatValue->contents);
                break;
            default:
                break;
            }
        }

        ret->lexemeValue = CreateString(env, j_array.dump().c_str());
    }

#ifdef BUILD_LISTENERS
    void coco::created_type(const type &tp) const
    {
        for (auto &l : listeners)
            l->created_type(tp);
    }
    void coco::created_item(const item &itm) const
    {
        for (auto &l : listeners)
            l->created_item(itm);
    }
    void coco::updated_item(const item &itm) const
    {
        for (auto &l : listeners)
            l->updated_item(itm);
    }
    void coco::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) const
    {
        for (auto &l : listeners)
            l->new_data(itm, data, timestamp);
    }
    void coco::created_reactive_rule(const reactive_rule &rr) const
    {
        for (auto &l : listeners)
            l->created_reactive_rule(rr);
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
