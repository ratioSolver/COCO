#include "coco_core.hpp"
#include "coco_db.hpp"
#include "logging.hpp"
#include <algorithm>
#include <cassert>

namespace coco
{
    coco_core::coco_core(std::unique_ptr<coco_db> &&db) : db(std::move(db)), env(CreateEnvironment())
    {
        assert(env != nullptr);
        AddUDF(env, "new_solver_script", "v", 2, 2, "ys", new_solver_script, "new_solver_script", this);
        AddUDF(env, "new_solver_rules", "v", 2, 2, "ym", new_solver_rules, "new_solver_rules", this);
        AddUDF(env, "start_execution", "v", 1, 1, "l", start_execution, "start_execution", this);
        AddUDF(env, "pause_execution", "v", 1, 1, "l", pause_execution, "pause_execution", this);
        AddUDF(env, "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", this);
        AddUDF(env, "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", this);
        AddUDF(env, "failure", "v", 2, 2, "lm", failure, "failure", this);
        AddUDF(env, "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", this);
        AddUDF(env, "adapt_files", "v", 2, 2, "lm", adapt_files, "adapt_files", this);
        AddUDF(env, "delete_solver", "v", 1, 1, "l", coco::delete_solver, "delete_solver", this);

        reset_knowledge_base();
    }

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

    type &coco_core::create_type(const std::string &name, const std::string &description, std::vector<std::reference_wrapper<const type>> &&parents, std::vector<std::unique_ptr<property>> &&static_properties, std::vector<std::unique_ptr<property>> &&dynamic_properties)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &st = db->create_type(*this, name, description, std::move(parents), std::move(static_properties), std::move(dynamic_properties));
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

    item &coco_core::create_item(const type &tp, const std::string &name, const json::json &props)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &[p_name, p] : tp.get_static_properties())
            if (!props.contains(p_name))
                LOG_WARN("Properties for new item " + name + " do not contain " + p_name + " from type " + tp.get_name());
            else if (!p->validate(props[p_name], schemas))
                LOG_WARN("Property " + p_name + " for type " + tp.get_name() + " is invalid");

        auto &s = db->create_item(*this, tp, name, props);
        new_item(s);
        return s;
    }

    void coco_core::delete_item(const item &itm)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_item(itm);
        deleted_item(itm.get_id());
    }

    void coco_core::add_data(const item &s, const json::json &data)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &[p_name, p] : s.get_type().get_dynamic_properties())
            if (!data.contains(p_name))
                LOG_WARN("Data for item " + s.get_id() + " do not contain " + p_name);
            else if (!p->validate(data[p_name], schemas))
                LOG_WARN("Data " + p_name + " for item " + s.get_id() + " is invalid");

        db->add_data(s, std::chrono::system_clock::now(), data);
        new_data(s, std::chrono::system_clock::now(), data);
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

    rule &coco_core::create_reactive_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_reactive_rule(*this, name, content);
        new_reactive_rule(r);
        return r;
    }

    std::vector<std::reference_wrapper<rule>> coco_core::get_deliberative_rules()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        return db->get_deliberative_rules();
    }

    rule &coco_core::create_deliberative_rule(const std::string &name, const std::string &content)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &r = db->create_deliberative_rule(*this, name, content);
        new_deliberative_rule(r);
        return r;
    }

    void coco_core::new_type([[maybe_unused]] const type &tp) {}
    void coco_core::updated_type([[maybe_unused]] const type &tp) {}
    void coco_core::deleted_type([[maybe_unused]] const std::string &tp_id) {}

    void coco_core::new_item([[maybe_unused]] const item &itm) {}
    void coco_core::updated_item([[maybe_unused]] const item &itm) {}
    void coco_core::deleted_item([[maybe_unused]] const std::string &itm_id) {}

    void coco_core::new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &data) {}

    void coco_core::new_solver([[maybe_unused]] const coco_executor &exec) {}
    void coco_core::deleted_solver([[maybe_unused]] const uintptr_t id) {}

    void coco_core::new_reactive_rule([[maybe_unused]] const rule &r) {}
    void coco_core::new_deliberative_rule([[maybe_unused]] const rule &r) {}

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
        Build(env, "(deftemplate item_type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))");
        Build(env, "(deftemplate is_a (slot type_id (type SYMBOL)) (slot parent_id (type SYMBOL)))");
        Build(env, "(deftemplate item (slot id (type SYMBOL)) (slot name (type STRING)))");
        Build(env, "(deftemplate is_instance_of (slot item_id (type SYMBOL)) (slot type_id (type SYMBOL)))");
        Build(env, "(defrule inheritance (is_a (type_id ?t) (parent_id ?p)) (is_instance_of (item_id ?i) (type_id ?t)) => (assert (is_instance_of (item_id ?i) (type_id ?p))))");
        Build(env, "(deftemplate solver (slot id (type INTEGER)) (slot purpose (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))");
        Build(env, "(deftemplate task (slot solver_id (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))");
        Build(env, "(deffunction tick () (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))");
        Build(env, "(deffunction starting (?solver_id ?task_type ?pars ?vals) (return TRUE))");
        Build(env, "(deffunction ending (?solver_id ?id) (return TRUE))");

        // we load the basic knowledge base..
        db->init(*this);

        Run(env, -1);
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
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->executor::start();
    }

    void pause_execution([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Pausing plan execution..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->executor::pause();
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

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

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

        coco_exec->dont_start_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
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

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

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

        coco_exec->dont_end_yet({std::pair<const ratio::atom *, utils::rational>(atm, delay)});
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

        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);
        coco_exec->failure(atms);
    }

    void adapt_script([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Adapting to RiDDLe snippet..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        UDFValue riddle;
        if (!UDFNextArgument(udfc, STRING_BIT, &riddle))
            return;

        try
        { // we adapt to the riddle script..
            coco_exec->adapt(riddle.lexemeValue->contents);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe script: " + std::string(e.what()));
        }
    }

    void adapt_files([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Adapting to RiDDLe files..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        UDFValue riddle_files;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &riddle_files))
            return;

        std::vector<std::string> fs;
        for (size_t i = 0; i < riddle_files.multifieldValue->length; ++i)
        {
            auto &file = riddle_files.multifieldValue->contents[i];
            if (file.header->type != STRING_TYPE)
                return;
            fs.push_back(file.lexemeValue->contents);
        }

        try
        { // we adapt to the riddle files..
            coco_exec->adapt(fs);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid RiDDLe files: " + std::string(e.what()));
        }
    }

    void delete_solver([[maybe_unused]] Environment *, UDFContext *udfc, [[maybe_unused]] UDFValue *)
    {
        LOG_DEBUG("Deleting solver..");

        UDFValue exec_ptr;
        if (!UDFFirstArgument(udfc, INTEGER_BIT, &exec_ptr))
            return;
        auto coco_exec = reinterpret_cast<coco_executor *>(exec_ptr.integerValue->contents);

        auto &cc = *reinterpret_cast<coco_core *>(udfc->context);
        cc.delete_solver(*coco_exec);
    }
} // namespace coco