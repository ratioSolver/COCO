#include "coco_executor.h"
#include "coco_core.h"
#include "coco_db.h"

namespace coco
{
    COCO_EXPORT coco_executor::coco_executor(coco_core &cc, ratio::executor::executor &exec) : core_listener(exec.get_solver()), solver_listener(exec.get_solver()), executor_listener(exec), cc(cc), exec(exec) {}

    void coco_executor::log([[maybe_unused]] const std::string &msg) {}
    void coco_executor::read([[maybe_unused]] const std::string &script) {}
    void coco_executor::read([[maybe_unused]] const std::vector<std::string> &files) {}

    void coco_executor::state_changed()
    {
        cc.fire_state_changed(*this);
    }

    void coco_executor::started_solving() { cc.fire_started_solving(*this); }
    void coco_executor::solution_found()
    {
        LOG_DEBUG("[" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "] Solution found..");
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        c_flaw = nullptr;
        c_resolver = nullptr;

        cc.fire_solution_found(*this);
    }
    void coco_executor::inconsistent_problem()
    {
        LOG_DEBUG("[" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "] Inconsistent problem..");
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        c_flaw = nullptr;
        c_resolver = nullptr;

        cc.fire_inconsistent_problem(*this);
    }

    void coco_executor::flaw_created(const ratio::flaw &f)
    {
        flaws.insert(&f);

        cc.fire_flaw_created(*this, f);
    }
    void coco_executor::flaw_state_changed([[maybe_unused]] const ratio::flaw &f)
    {
        cc.fire_flaw_state_changed(*this, f);
    }
    void coco_executor::flaw_cost_changed([[maybe_unused]] const ratio::flaw &f)
    {
        cc.fire_flaw_cost_changed(*this, f);
    }
    void coco_executor::flaw_position_changed([[maybe_unused]] const ratio::flaw &f)
    {
        cc.fire_flaw_position_changed(*this, f);
    }
    void coco_executor::current_flaw(const ratio::flaw &f)
    {
        c_flaw = &f;
        c_resolver = nullptr;

        cc.fire_current_flaw(*this, f);
    }

    void coco_executor::resolver_created(const ratio::resolver &r)
    {
        resolvers.insert(&r);

        cc.fire_resolver_created(*this, r);
    }
    void coco_executor::resolver_state_changed([[maybe_unused]] const ratio::resolver &r)
    {
        cc.fire_resolver_state_changed(*this, r);
    }
    void coco_executor::current_resolver(const ratio::resolver &r)
    {
        c_resolver = &r;

        cc.fire_current_resolver(*this, r);
    }

    void coco_executor::causal_link_added([[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r)
    {
        cc.fire_causal_link_added(*this, f, r);
    }

    void coco_executor::tick() { exec.tick(); }

    void coco_executor::executor_state_changed(ratio::executor::executor_state state)
    {
        LOG_DEBUG("[" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "] Executor state: " << ratio::executor::to_string(state));
        Eval(cc.env, ("(do-for-fact ((?slv solver)) (= ?slv:solver_ptr " + std::to_string(reinterpret_cast<uintptr_t>(this)) + ") (modify ?slv (state " + ratio::executor::to_string(state) + ")))").c_str(), NULL);
        cc.fire_executor_state_changed(*this, state);
        Run(cc.env, -1);
    }

    void coco_executor::tick(const utils::rational &time)
    {
        LOG_DEBUG("[" + std::to_string(reinterpret_cast<uintptr_t>(this)) + "] Current time: " << to_string(time));
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        current_time = time;

        cc.fire_tick(*this, time);
    }
    void coco_executor::starting(const std::unordered_set<ratio::atom *> &atoms)
    {
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        std::vector<Fact *> facts;
        for (const auto &atm : atoms)
            facts.push_back(AssertString(cc.env, to_task(*atm, "starting").c_str()));
        Run(cc.env, -1);
#ifdef VERBOSE_LOG
        Eval(cc.env, "(facts)", NULL);
#endif

        CLIPSValue res;
        Eval(cc.env, "(find-all-facts ((?f dont_start_yet)) TRUE)", &res);
        if (res.header->type == MULTIFIELD_TYPE && res.multifieldValue->length)
        {
            std::unordered_map<const ratio::atom *, utils::rational> dsy;
            std::vector<Fact *> dsy_facts;
            for (size_t i = 0; i < res.multifieldValue->length; ++i)
            {
                auto f = res.multifieldValue->contents[i].factValue;
                CLIPSValue atm_ptr, delay_time;
                GetFactSlot(f, "task_id", &atm_ptr);
                GetFactSlot(f, "delay_time", &delay_time);
                auto atm = reinterpret_cast<ratio::atom *>(atm_ptr.integerValue->contents);
                utils::rational delay;
                switch (delay_time.multifieldValue->length)
                {
                case 0:
                    delay = utils::rational::ONE;
                    break;
                case 1:
                    delay = utils::rational(delay_time.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    delay = utils::rational(delay_time.multifieldValue[0].contents->integerValue->contents, delay_time.multifieldValue[1].contents->integerValue->contents);
                    break;
                }
                dsy[atm] = delay;
                dsy_facts.push_back(f);
            }
            exec.dont_start_yet(dsy);

            for (const auto &f : dsy_facts)
                Retract(f);

            Run(cc.env, -1);
#ifdef VERBOSE_LOG
            Eval(cc.env, "(facts)", NULL);
#endif
        }

        // we retract the facts..
        for (const auto &f : facts)
            Retract(f);
        // we run the rules engine to update the policy..
        Run(cc.env, -1);
        // #ifdef VERBOSE_LOG
        //         Eval(env, "(facts)", NULL);
        // #endif
    }
    void coco_executor::start(const std::unordered_set<ratio::atom *> &atoms)
    {
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        executing_atoms.insert(atoms.cbegin(), atoms.cend());

        std::vector<Fact *> facts;
        for (const auto &atm : atoms)
            facts.push_back(AssertString(cc.env, to_task(*atm, "start").c_str()));
        Run(cc.env, -1);
#ifdef VERBOSE_LOG
        Eval(cc.env, "(facts)", NULL);
#endif
        cc.fire_start(*this, atoms);

        // we retract the facts..
        for (const auto &f : facts)
            Retract(f);
        // we run the rules engine to update the policy..
        Run(cc.env, -1);
        // #ifdef VERBOSE_LOG
        //         Eval(env, "(facts)", NULL);
        // #endif
    }

    void coco_executor::ending(const std::unordered_set<ratio::atom *> &atoms)
    {
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
#ifdef VERBOSE_LOG
        Eval(cc.env, "(facts)", NULL);
#endif
        std::vector<Fact *> facts;
        for (const auto &atm : atoms)
            facts.push_back(AssertString(cc.env, to_task(*atm, "ending").c_str()));
        Run(cc.env, -1);
#ifdef VERBOSE_LOG
        Eval(cc.env, "(facts)", NULL);
#endif

        CLIPSValue res;
        Eval(cc.env, "(find-all-facts ((?f dont_end_yet)) TRUE)", &res);
        if (res.multifieldValue->length)
        {
            std::unordered_map<const ratio::atom *, utils::rational> dey;
            std::vector<Fact *> dey_facts;
            for (size_t i = 0; i < res.multifieldValue->length; ++i)
            {
                auto f = res.multifieldValue->contents[i].factValue;
                CLIPSValue atm_ptr, delay_time;
                GetFactSlot(f, "task_id", &atm_ptr);
                GetFactSlot(f, "delay_time", &delay_time);
                auto atm = reinterpret_cast<ratio::atom *>(atm_ptr.integerValue->contents);
                utils::rational delay;
                switch (delay_time.multifieldValue->length)
                {
                case 0:
                    delay = utils::rational(1);
                    break;
                case 1:
                    delay = utils::rational(delay_time.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    delay = utils::rational(delay_time.multifieldValue[0].contents->integerValue->contents, delay_time.multifieldValue[1].contents->integerValue->contents);
                    break;
                }
                dey[atm] = delay;
                dey_facts.push_back(f);
            }
            exec.dont_end_yet(dey);

            for (const auto &f : dey_facts)
                Retract(f);

            Run(cc.env, -1);
#ifdef VERBOSE_LOG
            Eval(cc.env, "(facts)", NULL);
#endif
        }

        // we retract the facts..
        for (const auto &f : facts)
            Retract(f);
        // we run the rules engine to update the policy..
        Run(cc.env, -1);
        // #ifdef VERBOSE_LOG
        //         Eval(env, "(facts)", NULL);
        // #endif
    }
    void coco_executor::end(const std::unordered_set<ratio::atom *> &atoms)
    {
        const std::lock_guard<std::recursive_mutex> lock(cc.mtx);
        for (const auto &a : atoms)
            executing_atoms.erase(a);

        std::vector<Fact *> facts;
        for (const auto &atm : atoms)
            facts.push_back(AssertString(cc.env, to_task(*atm, "end").c_str()));
        Run(cc.env, -1);
#ifdef VERBOSE_LOG
        Eval(cc.env, "(facts)", NULL);
#endif
        cc.fire_end(*this, atoms);

        // we retract the facts..
        for (const auto &f : facts)
            Retract(f);
        // we run the rules engine to update the policy..
        Run(cc.env, -1);
        // #ifdef VERBOSE_LOG
        //         Eval(env, "(facts)", NULL);
        // #endif
    }

    std::string coco_executor::to_task(const ratio::atom &atm, const std::string &command)
    {
        std::string task_str = "(task (solver_ptr " + std::to_string(reinterpret_cast<uintptr_t>(this)) + ") (task_type " + atm.get_type().get_name() + ") (id " + std::to_string(get_id(atm)) + ") (command " + command + ")";
        std::string pars_str = "(pars";
        std::string vals_str = "(vals";

        for (const auto &[var_name, var] : atm.get_vars())
        {
            if (&var->get_type() == &slv.get_bool_type())
            {
                switch (slv.get_sat_core().value(static_cast<const ratio::bool_item &>(*var).get_lit()))
                {
                case utils::True:
                    pars_str += " " + var_name;
                    vals_str += " TRUE";
                    break;
                case utils::False:
                    pars_str += " " + var_name;
                    vals_str += " FALSE";
                    break;
                }
            }
            else if (&var->get_type() == &slv.get_real_type())
            {
                pars_str += " " + var_name;
                vals_str += " " + to_string(slv.get_lra_theory().value(static_cast<const ratio::arith_item &>(*var).get_lin()));
            }
            else if (&var->get_type() == &slv.get_time_type())
            {
                pars_str += " " + var_name;
                const auto [lb, ub] = slv.get_rdl_theory().bounds(static_cast<const ratio::arith_item &>(*var).get_lin());
                vals_str += " " + to_string(lb);
            }
            else if (&var->get_type() == &slv.get_string_type())
            {
                pars_str += " " + var_name;
                vals_str += " " + static_cast<const ratio::string_item &>(*var).get_string();
            }
            else if (auto ev = dynamic_cast<const ratio::enum_item *>(&*var))
            {
                const auto vals = slv.get_ov_theory().value(ev->get_var());
                if (vals.size() == 1)
                {
                    pars_str += " " + var_name;
                    vals_str += " " + slv.guess_name(dynamic_cast<riddle::item &>(**vals.begin()));
                }
            }
            else
            {
                pars_str += " " + var_name;
                vals_str += " " + slv.guess_name(*var);
            }
        }
        pars_str += ")";
        vals_str += ")";
        task_str += " " + pars_str + " " + vals_str + ")";

        return task_str;
    }

    COCO_EXPORT json::json to_state(const coco_executor &rhs) noexcept
    {
        json::json j_state{{"state", to_json(rhs.slv)}, {"timelines", to_timelines(rhs.slv)}, {"time", ratio::to_json(rhs.current_time)}};
        json::json j_executing(json::json_type::array);
        for (const auto &atm : rhs.executing_atoms)
            j_executing.push_back(get_id(*atm));
        j_state["executing"] = std::move(j_executing);
        return j_state;
    }
    COCO_EXPORT json::json to_graph(const coco_executor &rhs) noexcept
    {
        json::json j_graph;
        json::json j_flaws(json::json_type::array);
        for (const auto &f : rhs.flaws)
            j_flaws.push_back(to_json(*f));
        j_graph["flaws"] = std::move(j_flaws);
        if (rhs.c_flaw)
            j_graph["current_flaw"] = get_id(*rhs.c_flaw);
        json::json j_resolvers(json::json_type::array);
        for (const auto &r : rhs.resolvers)
            j_resolvers.push_back(to_json(*r));
        j_graph["resolvers"] = std::move(j_resolvers);
        if (rhs.c_resolver)
            j_graph["current_resolver"] = get_id(*rhs.c_resolver);
        return j_graph;
    }
} // namespace use
