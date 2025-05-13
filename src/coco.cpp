#include "coco.hpp"
#include "coco_module.hpp"
#include "coco_type.hpp"
#include "coco_property.hpp"
#include "coco_item.hpp"
#include "coco_db.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace coco
{
    coco::coco(coco_db &db) noexcept : db(db), env(CreateEnvironment())
    {
        add_property_type(utils::make_u_ptr<bool_property_type>(*this));
        add_property_type(utils::make_u_ptr<int_property_type>(*this));
        add_property_type(utils::make_u_ptr<float_property_type>(*this));
        add_property_type(utils::make_u_ptr<string_property_type>(*this));
        add_property_type(utils::make_u_ptr<symbol_property_type>(*this));
        add_property_type(utils::make_u_ptr<item_property_type>(*this));
        add_property_type(utils::make_u_ptr<json_property_type>(*this));

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
        LOG_TRACE(item_deftemplate);
        [[maybe_unused]] auto build_item_dt_err = Build(env, item_deftemplate);
        assert(build_item_dt_err == BE_NO_ERROR);
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
        for (auto &tp : tps)
            make_type(tp.name, {}, tp.static_props.has_value() ? std::move(*tp.static_props) : json::json{}, tp.dynamic_props.has_value() ? std::move(*tp.dynamic_props) : json::json{}, tp.data.has_value() ? std::move(*tp.data) : json::json{});

        for (auto &tp : tps)
            if (!tp.parents.empty())
            {
                std::vector<utils::ref_wrapper<const type>> parents;
                for (auto &parent : tp.parents)
                    parents.emplace_back(get_type(parent));
                get_type(tp.name).set_parents(std::move(parents));
            }

#ifdef BUILD_AUTH
        if (!types.count(user_kw))
            [[maybe_unused]]
            auto &usr_tp = create_type(user_kw, {}, {}, {}, {});
#endif

        LOG_DEBUG("Retrieving all reactive rules");
        auto rrs = db.get_reactive_rules();
        LOG_DEBUG("Retrieved " << rrs.size() << " reactive rules");
        for (auto &rule : rrs)
            reactive_rules.emplace(rule.name, utils::make_u_ptr<reactive_rule>(*this, rule.name, rule.content));

        LOG_DEBUG("Retrieving all items");
        auto itms = db.get_items();
        LOG_DEBUG("Retrieved " << itms.size() << " items");
        for (auto &itm : itms)
            get_type(itm.type).make_item(itm.id, itm.props.has_value() ? std::move(itm.props.value()) : json::json{});

        Run(env, -1);
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

#ifdef BUILD_AUTH
    std::string coco::get_token(std::string_view username, std::string_view password)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto user = db.get_user(username, password);
        return user.id;
    }

    item &coco::create_user(std::string_view username, std::string_view password, json::json &&personal_data)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &tp = get_type(user_kw);
        auto &itm = create_item(tp);
        db.create_user(itm.get_id(), username, password, std::move(personal_data));
        return itm;
    }
#endif

    std::vector<utils::ref_wrapper<type>> coco::get_types() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<utils::ref_wrapper<type>> res;
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

    type &coco::create_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent->get_name());
        db.create_type(name, parents_names, data, static_props, dynamic_props);
        auto &tp = make_type(name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
        if (infere)
            Run(env, -1);
        return tp;
    }
    void coco::set_parents(type &tp, std::vector<utils::ref_wrapper<const type>> &&parents, bool infere) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::string> parents_names;
        for (const auto &parent : parents)
            parents_names.emplace_back(parent->get_name());
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

    std::vector<utils::ref_wrapper<item>> coco::get_items() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<utils::ref_wrapper<item>> res;
        res.reserve(items.size());
        for (auto &i : items)
            res.push_back(*i.second);
        return res;
    }

    item &coco::get_item(std::string_view id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        if (items.find(id.data()) == items.end())
            throw std::invalid_argument("Type not found: " + std::string(id));
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

    std::vector<utils::ref_wrapper<reactive_rule>> coco::get_reactive_rules() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<utils::ref_wrapper<reactive_rule>> res;
        res.reserve(reactive_rules.size());
        for (auto &r : reactive_rules)
            res.push_back(*r.second);
        return res;
    }
    void coco::create_reactive_rule(std::string_view rule_name, std::string_view rule_content, bool infere)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db.create_reactive_rule(rule_name, rule_content);
        if (!reactive_rules.emplace(rule_name, utils::make_u_ptr<reactive_rule>(*this, rule_name, rule_content)).second)
            throw std::invalid_argument("reactive rule `" + std::string(rule_name) + "` already exists");
        if (infere)
            Run(env, -1);
    }

    void coco::add_property_type(utils::u_ptr<property_type> pt)
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

    type &coco::make_type(std::string_view name, std::vector<utils::ref_wrapper<const type>> &&parents, json::json &&static_props, json::json &&dynamic_props, json::json &&data)
    {
        auto tp_ptr = utils::make_u_ptr<type>(*this, name, std::move(parents), std::move(static_props), std::move(dynamic_props), std::move(data));
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

    json::json reactive_rule::to_json() const noexcept { return {{"content", content.c_str()}}; }

    json::json coco::to_json() const noexcept
    {
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
        return jc;
    }

    void add_data(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Adding data..");

        auto &cc = *reinterpret_cast<coco *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        auto &itm = *cc.items.at(item_id.lexemeValue->contents);
        std::map<std::string, utils::ref_wrapper<const property>> dynamic_props = itm.get_type().get_all_dynamic_properties();

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
            case SYMBOL_TYPE:
                if (std::string(val.lexemeValue->contents) == "TRUE")
                    data[par.lexemeValue->contents] = true;
                else if (std::string(val.lexemeValue->contents) == "FALSE")
                    data[par.lexemeValue->contents] = false;
                else if (dynamic_props.at(par.lexemeValue->contents)->is_complex())
                    data[par.lexemeValue->contents] = json::load(val.lexemeValue->contents);
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
    void coco::new_type(const type &tp) const
    {
        for (auto &l : listeners)
            l->new_type(tp);
    }
    void coco::new_item(const item &itm) const
    {
        for (auto &l : listeners)
            l->new_item(itm);
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
