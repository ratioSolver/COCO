#include "coco_deliberative.hpp"
#include "coco.hpp"
#include "deliberative_db.hpp"
#include "coco_executor.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    coco_deliberative::coco_deliberative(coco &cc) noexcept : coco_module(cc)
    {
        LOG_TRACE(executor_deftemplate);
        [[maybe_unused]] auto build_slv_dt_err = Build(get_env(), executor_deftemplate);
        assert(build_slv_dt_err == BE_NO_ERROR);
        LOG_TRACE(task_deftemplate);
        [[maybe_unused]] auto build_tsk_dt_err = Build(get_env(), task_deftemplate);
        assert(build_tsk_dt_err == BE_NO_ERROR);
        LOG_TRACE(tick_function);
        [[maybe_unused]] auto build_tck_fn_err = Build(get_env(), tick_function);
        assert(build_tck_fn_err == BE_NO_ERROR);
        LOG_TRACE(starting_function);
        [[maybe_unused]] auto build_strt_fn_err = Build(get_env(), starting_function);
        assert(build_strt_fn_err == BE_NO_ERROR);
        LOG_TRACE(ending_function);
        [[maybe_unused]] auto build_end_fn_err = Build(get_env(), ending_function);
        assert(build_end_fn_err == BE_NO_ERROR);

        [[maybe_unused]] auto create_executor_script_err = AddUDF(get_env(), "create_executor_script", "v", 2, 2, "ys", create_exec_script, "create_executor_script", this);
        assert(create_executor_script_err == AUE_NO_ERROR);
        [[maybe_unused]] auto create_executor_rules_err = AddUDF(get_env(), "create_executor_rules", "v", 2, 2, "ym", create_exec_rules, "create_executor_rules", this);
        assert(create_executor_rules_err == AUE_NO_ERROR);
        [[maybe_unused]] auto start_execution_err = AddUDF(get_env(), "start_execution", "v", 1, 1, "l", start_execution, "start_execution", this);
        assert(start_execution_err == AUE_NO_ERROR);
        [[maybe_unused]] auto delay_task_err = AddUDF(get_env(), "delay_task", "v", 2, 3, "llm", delay_task, "delay_task", this);
        assert(delay_task_err == AUE_NO_ERROR);
        [[maybe_unused]] auto extend_task_err = AddUDF(get_env(), "extend_task", "v", 2, 3, "llm", extend_task, "extend_task", this);
        assert(extend_task_err == AUE_NO_ERROR);
        [[maybe_unused]] auto failure_err = AddUDF(get_env(), "failure", "v", 2, 2, "lm", failure, "failure", this);
        assert(failure_err == AUE_NO_ERROR);
        [[maybe_unused]] auto adapt_script_err = AddUDF(get_env(), "adapt_script", "v", 2, 2, "ls", adapt_script, "adapt_script", this);
        assert(adapt_script_err == AUE_NO_ERROR);
        [[maybe_unused]] auto adapt_rules_err = AddUDF(get_env(), "adapt_rules", "v", 2, 2, "lm", adapt_rules, "adapt_rules", this);
        assert(adapt_rules_err == AUE_NO_ERROR);
        [[maybe_unused]] auto delete_executor_err = AddUDF(get_env(), "delete_executor", "v", 1, 1, "l", delete_exec, "delete_executor", this);

        LOG_DEBUG("Retrieving all deliberative rules");
        auto &db = get_coco().get_db().add_module<deliberative_db>(static_cast<mongo_db &>(get_coco().get_db()));
        auto drs = db.get_deliberative_rules();
        LOG_DEBUG("Retrieved " << drs.size() << " deliberative rules");
        for (auto &rule : drs)
            deliberative_rules.emplace(rule.name, utils::make_u_ptr<deliberative_rule>(*this, rule.name, rule.content));
    }

    std::vector<utils::ref_wrapper<deliberative_rule>> coco_deliberative::get_deliberative_rules() noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        std::vector<utils::ref_wrapper<deliberative_rule>> res;
        res.reserve(deliberative_rules.size());
        for (auto &r : deliberative_rules)
            res.push_back(*r.second);
        return res;
    }
    void coco_deliberative::create_deliberative_rule(std::string_view rule_name, std::string_view rule_content)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        get_coco().get_db().get_module<deliberative_db>().create_deliberative_rule(rule_name, rule_content);
        if (!deliberative_rules.emplace(rule_name, utils::make_u_ptr<deliberative_rule>(*this, rule_name, rule_content)).second)
            throw std::invalid_argument("deliberative rule `" + std::string(rule_name) + "` already exists");
    }

    deliberative_rule::deliberative_rule(coco_deliberative &cd, std::string_view name, std::string_view content) noexcept : cd(cd), name(name), content(content) {}

    json::json deliberative_rule::to_json() const noexcept { return {{"content", content.c_str()}}; }

    coco_executor &coco_deliberative::create_executor(std::string_view name)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        auto &exec = *executors.emplace(name, utils::make_u_ptr<coco_executor>(*this, name)).first->second.get();
        executor_created(exec);
        return exec;
    }
    void coco_deliberative::delete_executor(coco_executor &exec)
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        executors.erase(exec.get_name());
    }

    void create_exec_script(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Creating new executor..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto &exec = cc.create_executor(executor_name.lexemeValue->contents);

        UDFValue script;
        if (!UDFNextArgument(udfc, STRING_BIT, &script))
            return;

        exec.adapt(script.lexemeValue->contents);
    }
    void create_exec_rules(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Creating new executor..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto &exec = cc.create_executor(executor_name.lexemeValue->contents);

        UDFValue rules;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &rules))
            return;

        for (size_t i = 0; i < rules.multifieldValue->length; ++i)
        {
            auto &rule = rules.multifieldValue->contents[i];
            if (rule.header->type != SYMBOL_TYPE)
                return;
            auto it = cc.deliberative_rules.find(rule.lexemeValue->contents);
            if (it == cc.deliberative_rules.end())
                return;
            exec.adapt(it->second->get_content());
        }
    }
    void start_execution(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Starting execution..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        exec.plexa::start();
    }
    void delay_task(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Delaying task..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &task_id))
            return;

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

        exec.dont_start_yet({std::pair(atm, delay)});
    }
    void extend_task(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Extending task..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        UDFValue task_id;
        if (!UDFNextArgument(udfc, SYMBOL_BIT, &task_id))
            return;

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

        exec.dont_end_yet({std::pair(atm, delay)});
    }
    void failure(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Failure..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        UDFValue task_ids;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &task_ids))
            return;

        std::unordered_set<const riddle::atom_term *> atms;
        for (size_t i = 0; i < task_ids.multifieldValue->length; ++i)
        {
            auto &task_id = task_ids.multifieldValue->contents[i];
            if (task_id.header->type != SYMBOL_TYPE)
                return;
            auto atm = reinterpret_cast<const riddle::atom_term *>(task_id.integerValue->contents);
            atms.insert(atm);
        }

        exec.failure(atms);
    }
    void adapt_script(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Adapting script..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        UDFValue script;
        if (!UDFNextArgument(udfc, STRING_BIT, &script))
            return;

        exec.adapt(script.lexemeValue->contents);
    }
    void adapt_rules(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Adapting rules..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        UDFValue rules;
        if (!UDFNextArgument(udfc, MULTIFIELD_BIT, &rules))
            return;

        for (size_t i = 0; i < rules.multifieldValue->length; ++i)
        {
            auto &rule = rules.multifieldValue->contents[i];
            if (rule.header->type != SYMBOL_TYPE)
                return;
            auto it = cc.deliberative_rules.find(rule.lexemeValue->contents);
            if (it == cc.deliberative_rules.end())
                return;
            exec.adapt(it->second->get_content());
        }
    }
    void delete_exec(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Deleting executor..");

        auto &cc = *reinterpret_cast<coco_deliberative *>(udfc->context);

        UDFValue executor_name;
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &executor_name))
            return;
        auto it = cc.executors.find(executor_name.lexemeValue->contents);
        if (it == cc.executors.end())
            return;
        auto &exec = *it->second;

        cc.delete_executor(exec);
    }

#ifdef BUILD_LISTENERS
    void coco_deliberative::executor_created(coco_executor &exec)
    {
        for (auto &l : listeners)
            l->executor_created(exec);
    }
    void coco_deliberative::executor_deleted(coco_executor &exec)
    {
        for (auto &l : listeners)
            l->executor_deleted(exec);
    }

    void coco_deliberative::state_changed(coco_executor &exec)
    {
        for (auto &l : listeners)
            l->state_changed(exec);
    }

    void coco_deliberative::flaw_created(coco_executor &exec, const ratio::flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_created(exec, f);
    }
    void coco_deliberative::flaw_state_changed(coco_executor &exec, const ratio::flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_state_changed(exec, f);
    }
    void coco_deliberative::flaw_cost_changed(coco_executor &exec, const ratio::flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_cost_changed(exec, f);
    }
    void coco_deliberative::flaw_position_changed(coco_executor &exec, const ratio::flaw &f)
    {
        for (auto &l : listeners)
            l->flaw_position_changed(exec, f);
    }
    void coco_deliberative::current_flaw(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::flaw>> f)
    {
        for (auto &l : listeners)
            l->current_flaw(exec, f);
    }
    void coco_deliberative::resolver_created(coco_executor &exec, const ratio::resolver &r)
    {
        for (auto &l : listeners)
            l->resolver_created(exec, r);
    }
    void coco_deliberative::resolver_state_changed(coco_executor &exec, const ratio::resolver &r)
    {
        for (auto &l : listeners)
            l->resolver_state_changed(exec, r);
    }
    void coco_deliberative::current_resolver(coco_executor &exec, std::optional<utils::ref_wrapper<ratio::resolver>> r)
    {
        for (auto &l : listeners)
            l->current_resolver(exec, r);
    }
    void coco_deliberative::causal_link_added(coco_executor &exec, const ratio::flaw &f, const ratio::resolver &r)
    {
        for (auto &l : listeners)
            l->causal_link_added(exec, f, r);
    }

    void coco_deliberative::executor_state_changed(coco_executor &exec, ratio::executor::executor_state state)
    {
        for (auto &l : listeners)
            l->executor_state_changed(exec, state);
    }
    void coco_deliberative::tick(coco_executor &exec, const utils::rational &time)
    {
        for (auto &l : listeners)
            l->tick(exec, time);
    }
    void coco_deliberative::starting(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        for (auto &l : listeners)
            l->starting(exec, atms);
    }
    void coco_deliberative::start(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        for (auto &l : listeners)
            l->start(exec, atms);
    }
    void coco_deliberative::ending(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        for (auto &l : listeners)
            l->ending(exec, atms);
    }
    void coco_deliberative::end(coco_executor &exec, const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms)
    {
        for (auto &l : listeners)
            l->end(exec, atms);
    }

    deliberative_listener::deliberative_listener(coco_deliberative &cd) noexcept : cd(cd) { cd.listeners.emplace_back(this); }
    deliberative_listener::~deliberative_listener() { cd.listeners.erase(std::remove(cd.listeners.begin(), cd.listeners.end(), this), cd.listeners.end()); }
#endif

    void coco_deliberative::to_json(json::json &j) const noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        json::json j_execs(json::json_type::array);
        for (auto &e : executors)
            j_execs.push_back(e.second->to_json());
        j["executors"] = j_execs;
        json::json j_rules(json::json_type::array);
        for (auto &r : deliberative_rules)
            j_rules.push_back(r.second->to_json());
        j["deliberative_rules"] = j_rules;
    }
} // namespace coco
