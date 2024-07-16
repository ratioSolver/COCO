#include <cassert>
#include "coco_core.hpp"
#include "coco_db.hpp"
#include "logging.hpp"

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

    type &coco_core::create_type(const std::string &name, const std::string &description, std::map<std::string, std::unique_ptr<property>> &&static_pars, std::map<std::string, std::unique_ptr<property>> &&dynamic_pars)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        auto &st = db->create_type(name, description, std::move(static_pars), std::move(dynamic_pars));
        new_type(st);
        return st;
    }

    void coco_core::delete_type(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_type(db->get_type(id));
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

    item &coco_core::create_item(const type &tp, const std::string &name, const json::json &pars)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        for (const auto &[p_name, p] : tp.get_static_properties())
            if (!pars.contains(p_name))
                LOG_WARN("Parameters for type " + tp.get_id() + " do not contain " + p_name);
            else if (!p->validate(pars[p_name], schemas))
                LOG_WARN("Parameter " + p_name + " for type " + tp.get_id() + " is invalid");

        auto &s = db->create_item(tp, name, pars);
        new_item(s);
        return s;
    }

    void coco_core::delete_item(const std::string &id)
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        db->delete_item(db->get_item(id));
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
        auto &r = db->create_reactive_rule(name, content);
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
        auto &r = db->create_deliberative_rule(name, content);
        new_deliberative_rule(r);
        return r;
    }

    void coco_core::new_type([[maybe_unused]] const type &tp) { LOG_TRACE("New type: " + tp.get_id()); }
    void coco_core::updated_type([[maybe_unused]] const type &tp) { LOG_TRACE("Updated type: " + tp.get_id()); }
    void coco_core::deleted_type([[maybe_unused]] const std::string &tp_id) { LOG_TRACE("Deleted type: " + tp_id); }

    void coco_core::new_item([[maybe_unused]] const item &itm) { LOG_TRACE("New item: " + itm.get_id()); }
    void coco_core::updated_item([[maybe_unused]] const item &itm) { LOG_TRACE("Updated item: " + itm.get_id()); }
    void coco_core::deleted_item([[maybe_unused]] const std::string &itm_id) { LOG_TRACE("Deleted item: " + itm_id); }

    void coco_core::new_data([[maybe_unused]] const item &itm, [[maybe_unused]] const std::chrono::system_clock::time_point &timestamp, [[maybe_unused]] const json::json &data) { LOG_TRACE("Item " + itm.get_id() + " data: " + data.dump()); }

    void coco_core::new_solver([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("New solver: " + exec.get_solver().get_name() + " (" + std::to_string(get_id(exec)) + ")"); }
    void coco_core::deleted_solver([[maybe_unused]] const uintptr_t id) { LOG_TRACE("Deleted solver: " + std::to_string(id)); }

    void coco_core::new_reactive_rule([[maybe_unused]] const rule &r) { LOG_TRACE("New reactive rule: " + r.get_id()); }
    void coco_core::new_deliberative_rule([[maybe_unused]] const rule &r) { LOG_TRACE("New deliberative rule: " + r.get_id()); }

    void coco_core::state_changed([[maybe_unused]] const coco_executor &exec) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " is now " + to_string(exec.get_state())); }

    void coco_core::flaw_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " found a flaw: " + to_string(f.get_phi())); }
    void coco_core::flaw_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " is now " + to_state(f)); }
    void coco_core::flaw_cost_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " cost changed to " + to_string(f.get_estimated_cost())); }
    void coco_core::flaw_position_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Flaw " + to_string(f.get_phi()) + " position changed to " + std::to_string(exec.get_solver().get_idl_theory().bounds(f.get_position()).first)); }
    void coco_core::current_flaw([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f) { LOG_TRACE("Current flaw: " + to_string(f.get_phi())); }

    void coco_core::resolver_created([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " created a resolver: " + to_string(r.get_rho())); }
    void coco_core::resolver_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Resolver " + to_string(r.get_rho()) + " is now " + to_state(r)); }
    void coco_core::current_resolver([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Current resolver: " + to_string(r.get_rho())); }

    void coco_core::causal_link_added([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) { LOG_TRACE("Causal link added: " + to_string(f.get_phi()) + " -> " + to_string(r.get_rho())); }

    void coco_core::executor_state_changed([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] ratio::executor::executor_state state) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " is now " + to_string(state)); }

    void coco_core::tick([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const utils::rational &time) { LOG_TRACE("Solver " + exec.get_solver().get_name() + " ticked at " + to_string(time)); }

    void coco_core::starting([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " starting atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::start([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " started atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::ending([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " ending atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::end([[maybe_unused]] const coco_executor &exec, [[maybe_unused]] const std::vector<std::reference_wrapper<const ratio::atom>> &atoms)
    {
#if LOGGING_LEVEL >= LOG_TRACE_LEVEL
        std::string str = "Solver " + exec.get_solver().get_name() + " ended atoms: ";
        for (const auto &a : atoms)
            str += std::to_string(get_id(a.get())) + " ";
        LOG_TRACE(str);
#endif
    }

    void coco_core::reset_knowledge_base()
    {
        std::lock_guard<std::recursive_mutex> _(mtx);
        Clear(env);
        AssertString(env, std::string("(deftemplate item_type (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))").c_str());
        AssertString(env, std::string("(deftemplate item (slot id (type SYMBOL)) (slot name (type STRING)) (slot description (type STRING)))").c_str());
        AssertString(env, std::string("(deftemplate is_instance_of (slot item_id (type SYMBOL)) (slot type_id (type SYMBOL)))").c_str());
        AssertString(env, std::string("(deftemplate solver (slot id (type INTEGER)) (slot purpose (type SYMBOL)) (slot state (allowed-values reasoning idle adapting executing finished failed)))").c_str());
        AssertString(env, std::string("(deftemplate task (slot solver_id (type INTEGER)) (slot id (type INTEGER)) (slot task_type (type SYMBOL)) (multislot pars (type SYMBOL)) (multislot vals) (slot since (type INTEGER) (default 0)))").c_str());
        AssertString(env, std::string("(deffunction tick () (do-for-all-facts ((?task task)) TRUE (modify ?task (since (+ ?task:since 1)))) (return TRUE))").c_str());
        AssertString(env, std::string("(deffunction starting (?id ?task_type ?pars ?vals) (return TRUE))").c_str());
        AssertString(env, std::string("(deffunction ending (?id ?id) (return TRUE))").c_str());

        for (auto &tp : db->get_types())
        {
            FactBuilder *type_fact_builder = CreateFactBuilder(env, "item_type");
            FBPutSlotSymbol(type_fact_builder, "id", tp.get().get_id().c_str());
            FBPutSlotString(type_fact_builder, "name", tp.get().get_name().c_str());
            FBPutSlotString(type_fact_builder, "description", tp.get().get_description().c_str());
            FBAssert(type_fact_builder);
            FBDispose(type_fact_builder);

            for (const auto &[p_name, p] : tp.get().get_static_properties())
                AssertString(env, p.get()->to_deftemplate(tp.get(), true).c_str());
            for (const auto &[p_name, p] : tp.get().get_dynamic_properties())
                AssertString(env, p.get()->to_deftemplate(tp.get(), false).c_str());
        }

        for (auto &itm : db->get_items())
        {
            FactBuilder *item_fact_builder = CreateFactBuilder(env, "item");
            FBPutSlotSymbol(item_fact_builder, "id", itm.get().get_id().c_str());
            FBPutSlotString(item_fact_builder, "name", itm.get().get_name().c_str());
            FBPutSlotString(item_fact_builder, "description", itm.get().get_type().get_description().c_str());
            FBAssert(item_fact_builder);
            FBDispose(item_fact_builder);

            FactBuilder *is_instance_of_fact_builder = CreateFactBuilder(env, "is_instance_of");
            FBPutSlotSymbol(is_instance_of_fact_builder, "item_id", itm.get().get_id().c_str());
            FBPutSlotSymbol(is_instance_of_fact_builder, "type_id", itm.get().get_type().get_id().c_str());
            FBAssert(is_instance_of_fact_builder);
            FBDispose(is_instance_of_fact_builder);

            for (const auto &[p_name, p] : itm.get().get_type().get_static_properties())
            {
                FactBuilder *item_fact_builder = CreateFactBuilder(env, (itm.get().get_type().get_name() + "_has_" + p_name).c_str());
                FBPutSlotSymbol(item_fact_builder, "item_id", itm.get().get_id().c_str());
                p->set_value(item_fact_builder, itm.get().get_properties()[p_name]);
                FBAssert(item_fact_builder);
                FBDispose(item_fact_builder);
            }
        }

        for (auto &r_rule : db->get_reactive_rules())
            AssertString(env, r_rule.get().get_content().c_str());
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