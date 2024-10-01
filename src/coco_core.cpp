#include "coco_core.hpp"
#include "coco_db.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace coco
{
#ifdef ENABLE_TRANSFORMER
    coco_core::coco_core(std::unique_ptr<coco_db> &&db, const std::string &host, unsigned short port) : client(host, port), db(std::move(db)), env(CreateEnvironment())
#else
    coco_core::coco_core(std::unique_ptr<coco_db> &&db) : db(std::move(db)), env(CreateEnvironment())
#endif
    {
        assert(env != nullptr);
        AddUDF(env, "add_data", "v", 3, 4, "ymml", coco::add_data, "add_data", this);
        AddUDF(env, "new_solver_script", "v", 2, 2, "ys", new_solver_script, "new_solver_script", this);
        AddUDF(env, "new_solver_rules", "v", 2, 2, "ym", new_solver_rules, "new_solver_rules", this);
        AddUDF(env, "start_execution", "v", 1, 1, "l", start_execution, "start_execution", this);
        AddUDF(env, "pause_execution", "v", 1, 1, "l", pause_execution, "pause_execution", this);
        AddUDF(env, "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", this);
        AddUDF(env, "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", this);
        AddUDF(env, "failure", "v", 2, 2, "lm", failure, "failure", this);
        AddUDF(env, "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", this);
        AddUDF(env, "adapt_rules", "v", 2, 2, "lm", adapt_rules, "adapt_rules", this);
        AddUDF(env, "delete_solver", "v", 1, 1, "l", coco::delete_solver, "delete_solver", this);

#ifdef ENABLE_TRANSFORMER
        AddUDF(env, "understand", "v", 1, 1, "s", understand, "understand", this);
        AddUDF(env, "trigger_intent", "v", 2, 5, "yymms", trigger_intent, "trigger_intent", this);
        AddUDF(env, "compute_response", "v", 2, 3, "yss", compute_response, "compute_response", this);

        auto res = client.get("/version");
        if (!res || res->get_status_code() != network::ok)
            LOG_ERR("Failed to connect to the transformer server");
        else
            LOG_DEBUG("Connected to the transformer server " << static_cast<network::json_response &>(*res).get_body());
#endif

        reset_knowledge_base();
    }

#ifdef ENABLE_AUTH
    [[nodiscard]] item &coco_core::create_user(const std::string &username, const std::string &password, json::json &&data) { return db->create_user(*this, username, password, std::move(data)); }
    [[nodiscard]] std::optional<std::reference_wrapper<item>> coco_core::get_user(const std::string &username, const std::string &password) { return db->get_user(username, password); }
    void coco_core::set_user_username(item &usr, const std::string &username) { db->set_user_username(usr, username); }
    void coco_core::set_user_password(item &usr, const std::string &password) { db->set_user_password(usr, password); }
#endif

    std::vector<std::reference_wrapper<type>> coco_core::get_types()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_types();
    }

    type &coco_core::get_type(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_type(id);
    }

    type &coco_core::get_type_by_name(const std::string &name)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_type_by_name(name);
    }

    type &coco_core::create_type(const std::string &name, const std::string &description, json::json &&props, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &st = db->create_type(*this, name, description, std::move(props), std::move(parents), std::move(static_properties), std::move(dynamic_properties));
        new_type(st);
        return st;
    }

    void coco_core::set_type_name(type &tp, const std::string &name)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_type_name(tp, name);
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::set_type_description(type &tp, const std::string &description)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_type_description(tp, description);
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::set_type_properties(type &tp, json::json &&props)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_type_properties(tp, std::move(props));
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::add_parent(type &tp, const type &parent)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->add_parent(tp, parent);
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::remove_parent(type &tp, const type &parent)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->remove_parent(tp, parent);
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::add_static_property(type &tp, std::unique_ptr<property> &&prop)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->add_static_property(tp, std::move(prop));
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::remove_static_property(type &tp, const property &prop)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->remove_static_property(tp, prop);
        updated_type(tp);
    }

    void coco_core::add_dynamic_property(type &tp, std::unique_ptr<property> &&prop)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->add_dynamic_property(tp, std::move(prop));
        Run(env, -1);
        updated_type(tp);
    }

    void coco_core::remove_dynamic_property(type &tp, const property &prop)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->remove_dynamic_property(tp, prop);
        updated_type(tp);
    }

    void coco_core::delete_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_type(tp);
        deleted_type(tp.get_id());
    }

    std::vector<std::reference_wrapper<item>> coco_core::get_items_by_type(const type &tp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<item>> res;
        FunctionCallBuilder *all_instances_of_builder = CreateFunctionCallBuilder(env, 1);
        FCBAppendSymbol(all_instances_of_builder, tp.get_id().c_str());
        CLIPSValue instances;
        FCBCall(all_instances_of_builder, "get-all-instances-of", &instances); // we call the function
        FCBDispose(all_instances_of_builder);
        if (instances.header->type == MULTIFIELD_TYPE)
            for (size_t i = 0; i < instances.multifieldValue->length; ++i)
                res.push_back(get_item(instances.multifieldValue->contents[i].lexemeValue->contents));
        return res;
    }

    std::vector<std::reference_wrapper<item>> coco_core::get_items()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_items();
    }

    item &coco_core::get_item(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_item(id);
    }

    item &coco_core::create_item(const type &tp, json::json &&props)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &[p_name, p] : tp.get_static_properties())
            if (!props.contains(p_name))
                LOG_WARN("Properties for new item do not contain " + p_name + " from type " + tp.get_name());
            else if (!p->validate(props[p_name], schemas))
                LOG_WARN("Property " + p_name + " for type " + tp.get_name() + " is invalid");

        auto &s = db->create_item(*this, tp, std::move(props));
        new_item(s);
        return s;
    }

    void coco_core::set_item_properties(item &itm, json::json &&props)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &[p_name, p] : itm.get_type().get_static_properties())
            if (!props.contains(p_name))
                LOG_WARN("Properties for item " + itm.get_id() + " do not contain " + p_name);
            else if (!p->validate(props[p_name], schemas))
                LOG_WARN("Property " + p_name + " for item " + itm.get_id() + " is invalid");

        db->set_item_properties(itm, std::move(props));
        updated_item(itm);
    }

    void coco_core::set_item_value(item &itm, const json::json &value, const std::chrono::system_clock::time_point &timestamp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_item_value(itm, value, timestamp);
        new_data(itm, value, timestamp);
        Run(env, -1);
    }

    void coco_core::delete_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_item(itm);
        deleted_item(itm.get_id());
    }

    json::json coco_core::get_data(const item &itm, const std::chrono::system_clock::time_point &from, const std::chrono::system_clock::time_point &to)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_data(itm, from, to);
    }

    void coco_core::add_data(item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        set_item_value(itm, data, timestamp);
        db->add_data(itm, data, timestamp);
    }

    std::vector<std::reference_wrapper<coco_executor>> coco_core::get_solvers()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<std::reference_wrapper<coco_executor>> res;
        for (auto &exec : executors)
            res.push_back(*exec);
        return res;
    }

    coco_executor &coco_core::create_solver(const std::string &name, const utils::rational &units_per_tick)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &exec_ref = *executors.insert(std::make_unique<coco_executor>(*this, name, units_per_tick)).first->get();
        new_solver(exec_ref);
        exec_ref.init();
        return exec_ref;
    }

    void coco_core::delete_solver(coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto id = get_id(exec);
        auto it = std::find_if(executors.begin(), executors.end(), [id](const std::unique_ptr<coco_executor> &exec)
                               { return get_id(*exec) == id; });
        executors.erase(it);
        deleted_solver(id);
    }

    std::vector<std::reference_wrapper<rule>> coco_core::get_reactive_rules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_reactive_rules();
    }

    rule &coco_core::get_reactive_rule(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_reactive_rule(id);
    }

    rule &coco_core::create_reactive_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_reactive_rule(*this, name, content);
        new_reactive_rule(r);
        return r;
    }

    void coco_core::set_reactive_rule_name(rule &r, const std::string &name)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_reactive_rule_name(r, name);
        Run(env, -1);
        updated_reactive_rule(r);
    }

    void coco_core::set_reactive_rule_content(rule &r, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_reactive_rule_content(r, content);
        Run(env, -1);
        updated_reactive_rule(r);
    }

    void coco_core::delete_reactive_rule(rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_reactive_rule(r);
        deleted_reactive_rule(r);
    }

    std::vector<std::reference_wrapper<rule>> coco_core::get_deliberative_rules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_deliberative_rules();
    }

    rule &coco_core::get_deliberative_rule(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_deliberative_rule(id);
    }

    rule &coco_core::create_deliberative_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_deliberative_rule(*this, name, content);
        new_deliberative_rule(r);
        return r;
    }

    void coco_core::set_deliberative_rule_name(rule &r, const std::string &name)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_deliberative_rule_name(r, name);
        Run(env, -1);
        updated_deliberative_rule(r);
    }

    void coco_core::set_deliberative_rule_content(rule &r, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->set_deliberative_rule_content(r, content);
        Run(env, -1);
        updated_deliberative_rule(r);
    }

    void coco_core::delete_deliberative_rule(rule &r)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_deliberative_rule(r);
        deleted_deliberative_rule(r);
    }

    std::vector<Deftemplate *> coco_core::get_deftemplates()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<Deftemplate *> res;
        Deftemplate *deftemplate = nullptr;
        deftemplate = GetNextDeftemplate(env, deftemplate);
        if (deftemplate)
            do
            {
                res.push_back(deftemplate);
            } while ((deftemplate = GetNextDeftemplate(env, deftemplate)) != NULL);
        return res;
    }
    std::vector<Defrule *> coco_core::get_defrules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<Defrule *> res;
        Defrule *defrule = nullptr;
        defrule = GetNextDefrule(env, defrule);
        if (defrule)
            do
            {
                res.push_back(defrule);
            } while ((defrule = GetNextDefrule(env, defrule)) != NULL);
        return res;
    }
    std::vector<Fact *> coco_core::get_facts()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        std::vector<Fact *> res;
        Fact *fact = nullptr;
        fact = GetNextFact(env, fact);
        if (fact)
            do
            {
                res.push_back(fact);
            } while ((fact = GetNextFact(env, fact)) != NULL);
        return res;
    }

    void coco_core::new_type([[maybe_unused]] const type &tp) {}
    void coco_core::updated_type([[maybe_unused]] const type &tp) {}
    void coco_core::deleted_type([[maybe_unused]] const std::string &tp_id) {}

    void coco_core::new_item([[maybe_unused]] const item &itm) {}
    void coco_core::updated_item([[maybe_unused]] const item &itm) {}
    void coco_core::deleted_item([[maybe_unused]] const std::string &itm_id) {}

    void coco_core::new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const json::json &data, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp) {}

    void coco_core::new_solver([[maybe_unused]] const coco_executor &exec) {}
    void coco_core::deleted_solver([[maybe_unused]] const uintptr_t id) {}

    void coco_core::new_reactive_rule([[maybe_unused]] const rule &r) {}
    void coco_core::updated_reactive_rule([[maybe_unused]] const rule &r) {}
    void coco_core::deleted_reactive_rule([[maybe_unused]] const rule &r) {}
    void coco_core::new_deliberative_rule([[maybe_unused]] const rule &r) {}
    void coco_core::updated_deliberative_rule([[maybe_unused]] const rule &r) {}
    void coco_core::deleted_deliberative_rule([[maybe_unused]] const rule &r) {}

    void coco_core::state_changed([[maybe_unused]] const coco_executor &exec) {}

    void coco_core::flaw_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    void coco_core::flaw_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    void coco_core::flaw_cost_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    void coco_core::flaw_position_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}
    void coco_core::current_flaw([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) {}

    void coco_core::resolver_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    void coco_core::resolver_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}
    void coco_core::current_resolver([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) {}

    void coco_core::causal_link_added([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) {}

    void coco_core::executor_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] ratio::executor::executor_state state) {}

    void coco_core::tick([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const utils::rational &time) {}

    void coco_core::starting([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms) {}
    void coco_core::start([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms) {}

    void coco_core::ending([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms) {}
    void coco_core::end([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms) {}

    void coco_core::reset_knowledge_base()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        Clear(env);

        // we build the basic knowledge base..
        Build(env, "(deftemplate type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))");
        Build(env, "(deftemplate is_a (slot type_id (type SYMBOL)) (slot parent_id (type SYMBOL)))");
        Build(env, "(deftemplate item (slot id (type SYMBOL)))");
        Build(env, "(deftemplate is_instance_of (slot item_id (type SYMBOL)) (slot type_id (type SYMBOL)))");
        Build(env, "(defrule inheritance (is_a (type_id ?t) (parent_id ?p)) (is_instance_of (item_id ?i) (type_id ?t)) => (assert (is_instance_of (item_id ?i) (type_id ?p))))");
        Build(env, "(deffunction get-all-instances-of (?type_id) (bind ?instances (create$)) (do-for-all-facts ((?is_instance_of is_instance_of)) (eq ?is_instance_of:type_id ?type_id) (bind ?instances (create$ ?instances ?is_instance_of:item_id))) (return ?instances))");

        // we build the basic knowledge base for the solvers..
        Build(env, "(deftemplate solver (slot id (type INTEGER)) (slot purpose (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))");
        Build(env, "(deftemplate task (slot solver_id (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))");
        Build(env, "(deffunction tick () (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))");
        Build(env, "(deffunction starting (?solver_id ?task_type ?pars ?vals) (return TRUE))");
        Build(env, "(deffunction ending (?solver_id ?id) (return TRUE))");

#ifdef ENABLE_TRANSFORMER
        // we build the basic knowledge base for the transformer..
        Build(env, "(deffunction intent (?intent ?confidence ?entities ?values ?confidences) (return TRUE))");
#endif

#ifdef ENABLE_AUTH
        // we build the basic knowledge base for the users..
        if (!db->has_type_name("User"))
        {
            std::vector<std::unique_ptr<property>> props;
            props.push_back(std::make_unique<integer_property>("role", "The role of the user", 0));
            create_type("User", "A CoCo user", {}, {}, std::move(props), {});

            LOG_WARN("Creating default admin user. Please change the password immediately.");
            db->create_user(*this, "admin", "admin", {{"role", roles::admin}});
        }
#endif

        // we load the basic knowledge base..
        db->init(*this);

        Run(env, -1);
    }

    void add_data(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Adding data..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        auto &itm = cc.get_item(item_id.lexemeValue->contents);

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
            cc.add_data(itm, data, std::chrono::system_clock::time_point(std::chrono::milliseconds(timestamp.integerValue->contents)));
        }
        else
            cc.add_data(itm, data);
    }

    void new_solver_script(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Creating new solver..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue solver_purpose;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &solver_purpose))
            return;
        auto &exec = cc.create_solver(solver_purpose.lexemeValue->contents, utils::rational::one);

        UDFValue riddle_script;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle_script))
            return;
        try
        {
            exec.adapt(riddle_script.lexemeValue->contents);
        }
        catch (const std::exception &e)
        {
            LOG_ERR("Invalid RiDDLe script: " + std::string(e.what()));
        }
    }

    void new_solver_rules(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Creating new solver..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue solver_purpose;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &solver_purpose))
            return;
        auto &exec = cc.create_solver(solver_purpose.lexemeValue->contents, utils::rational::one);

        UDFValue deliberative_rule_names;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &deliberative_rule_names))
            return;

        for (size_t i = 0; i < deliberative_rule_names.multifieldValue->length; ++i)
        {
            auto &rule_name = deliberative_rule_names.multifieldValue->contents[i];
            if (rule_name.header->type != STRING_TYPE)
                return;
            try
            {
                exec.adapt(cc.db->get_reactive_rule_by_name(rule_name.lexemeValue->contents).get_content());
            }
            catch (const std::exception &e)
            {
                LOG_ERR("Invalid RiDDLe files: " + std::string(e.what()));
            }
        }
    }

    void start_execution([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Starting plan execution..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        exec->executor::start();
    }

    void pause_execution([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Pausing plan execution..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        exec->executor::pause();
    }

    void delay_task([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Delaying task..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, INTEGER_BIT, &task_id))
            return;

        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        auto atm = reinterpret_cast<ratio::atom *>(task_id.integerValue->contents);
        utils::rational delay = utils::rational::one;
        if (UDFHasNextArgument(udfc))
        { // we have a delay..
            UDFValue delay_val;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
                return;
            switch (delay_val.multifieldValue->length)
            {
            case 1:
            {
                auto &num = delay_val.multifieldValue->contents[0];
                if (num.header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num.integerValue->contents);
                break;
            }
            case 2:
            {
                auto &num = delay_val.multifieldValue->contents[0];
                if (num.header->type != INTEGER_TYPE)
                    return;
                auto &den = delay_val.multifieldValue->contents[1];
                if (den.header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num.integerValue->contents, den.integerValue->contents);
                break;
            }
            default:
                return;
            }
        }

        exec->dont_start_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
    }

    void extend_task([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Extending task..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, INTEGER_BIT, &task_id))
            return;

        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        auto atm = reinterpret_cast<ratio::atom *>(task_id.integerValue->contents);
        utils::rational delay = utils::rational::one;
        if (UDFHasNextArgument(udfc))
        { // we have a delay..
            UDFValue delay_val;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &delay_val))
                return;
            switch (delay_val.multifieldValue->length)
            {
            case 1:
            {
                auto &num = delay_val.multifieldValue->contents[0];
                if (num.header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num.integerValue->contents);
                break;
            }
            case 2:
            {
                auto &num = delay_val.multifieldValue->contents[0];
                if (num.header->type != INTEGER_TYPE)
                    return;
                auto &den = delay_val.multifieldValue->contents[1];
                if (den.header->type != INTEGER_TYPE)
                    return;
                delay = utils::rational(num.integerValue->contents, den.integerValue->contents);
                break;
            }
            default:
                return;
            }
        }

        exec->dont_end_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
    }

    void failure([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Task(s) failure..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;

        UDFValue task_ids;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &task_ids))
            return;

        std::unordered_set<const ratio::atom *> atms;
        for (size_t i = 0; i < task_ids.multifieldValue->length; ++i)
        {
            auto &atm_id = task_ids.multifieldValue->contents[i];
            if (atm_id.header->type != INTEGER_TYPE)
                return;
            atms.insert(reinterpret_cast<ratio::atom *>(atm_id.integerValue->contents));
        }

        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        exec->failure(atms);
    }

    void adapt_script([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Adapting to RiDDLe snippet..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        try
        { // we adapt to the riddle script..
            exec->adapt(riddle.lexemeValue->contents);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe script: " + std::string(e.what()));
        }
    }

    void adapt_rules([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Adapting to RiDDLe rules..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        UDFValue riddle_files;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle_files))
            return;

        UDFValue deliberative_rule_names;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &deliberative_rule_names))
            return;

        for (size_t i = 0; i < deliberative_rule_names.multifieldValue->length; ++i)
        {
            auto &rule_name = deliberative_rule_names.multifieldValue->contents[i];
            if (rule_name.header->type != STRING_TYPE)
                return;
            try
            {
                exec->adapt(cc.db->get_reactive_rule_by_name(rule_name.lexemeValue->contents).get_content());
            }
            catch (const std::exception &e)
            {
                LOG_ERR("Invalid RiDDLe files: " + std::string(e.what()));
            }
        }
    }

    void delete_solver([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Deleting solver..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);
        cc.delete_solver(*exec);
    }

#ifdef ENABLE_TRANSFORMER
    void understand(Environment *env, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Understanding..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue message;
        if (!UDFFirstArgument(udfc, STRING_BIT, &message))
            return;

        auto res = cc.client.post("/model/parse", {{"text", message.lexemeValue->contents}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        std::string intent = json_res.get_body()["intent"]["name"];
        double confidence = json_res.get_body()["intent"]["confidence"];
        auto &entities = json_res.get_body()["entities"].as_array();

        FunctionCallBuilder *intent_builder = CreateFunctionCallBuilder(env, 5);
        FCBAppendSymbol(intent_builder, intent.c_str());
        FCBAppendFloat(intent_builder, confidence);
        auto es = CreateMultifieldBuilder(env, entities.size());
        auto vs = CreateMultifieldBuilder(env, entities.size());
        auto cs = CreateMultifieldBuilder(env, entities.size());
        for (const auto &entity : entities)
        {
            MBAppendSymbol(es, static_cast<std::string>(entity["entity"]).c_str());
            MBAppendSymbol(vs, static_cast<std::string>(entity["value"]).c_str());
            MBAppendFloat(cs, entity["confidence"]);
        }
        FCBAppendMultifield(intent_builder, MBCreate(es));
        FCBAppendMultifield(intent_builder, MBCreate(vs));
        FCBAppendMultifield(intent_builder, MBCreate(cs));
        FCBCall(intent_builder, "intent", nullptr);
        FCBDispose(intent_builder);
    }
    void trigger_intent(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_TRACE("Triggering intent..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue item;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item))
            return;

        UDFValue intent;
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &intent))
            return;

        auto &itm = cc.get_item(item.lexemeValue->contents);
        LOG_DEBUG(itm.get_id() << " triggers intent: " << intent.lexemeValue->contents);

        json::json body{{"name", intent.lexemeValue->contents}};

        if (UDFHasNextArgument(udfc))
        {
            UDFValue entities, values;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &entities))
                return;
            if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &values))
                return;

            if (entities.multifieldValue->length != values.multifieldValue->length)
                return;

            json::json entities_json;
            for (size_t i = 0; i < entities.multifieldValue->length; ++i)
            {
                auto &entity = entities.multifieldValue->contents[i];
                if (entity.header->type != SYMBOL_TYPE)
                    return;
                auto &value = values.multifieldValue->contents[i];
                switch (value.header->type)
                {
                case INTEGER_TYPE:
                    entities_json[entity.lexemeValue->contents] = static_cast<int64_t>(value.integerValue->contents);
                    break;
                case FLOAT_TYPE:
                    entities_json[entity.lexemeValue->contents] = value.floatValue->contents;
                    break;
                case STRING_TYPE:
                case SYMBOL_TYPE:
                    entities_json[entity.lexemeValue->contents] = value.lexemeValue->contents;
                    break;
                default:
                    return;
                }
            }
            LOG_DEBUG("Entities: " << entities_json.dump());
            body["entities"] = std::move(entities_json);
        }

        std::string url = "/conversations/";
        url += item.lexemeValue->contents;
        url += "/trigger_intent";
        auto res = cc.client.post(std::move(url), std::move(body));
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to trigger intent..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        auto &messages = json_res.get_body()["messages"].as_array();
        for (auto &message : messages)
        {
            json::json data;
            for (auto &[key, value] : message.as_object())
                if (key != "recipient_id")
                    data[key] = value;
            data["me"] = false;
            cc.add_data(itm, data);
        }
    }
    void compute_response(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_TRACE("Computing response..");

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);

        UDFValue item;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item))
            return;

        UDFValue message;
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        auto &itm = cc.get_item(item.lexemeValue->contents);
        LOG_DEBUG(itm.get_id() << " says: '" << message.lexemeValue->contents << "'");

        auto res = cc.client.post("/webhooks/rest/webhook", {{"sender", item.lexemeValue->contents}, {"message", message.lexemeValue->contents}}, {{"Content-Type", "application/json"}, {"Connection", "keep-alive"}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to compute response..");
            return;
        }

        auto &json_res = static_cast<network::json_response &>(*res);
        for (auto &response : json_res.get_body().as_array())
        {
            json::json data;
            for (auto &[key, value] : response.as_object())
                if (key != "recipient_id")
                    data[key] = value;
            data["me"] = false;
            cc.add_data(itm, data);
        }
    }
#endif
} // namespace coco