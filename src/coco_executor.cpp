#include "coco_executor.h"
#include "coco_core.h"
#include "coco_db.h"
#include <cstring>

namespace coco
{
    COCO_EXPORT coco_executor::coco_executor(coco_core &cc, ratio::executor::executor &exec) : core_listener(exec.get_solver()), solver_listener(exec.get_solver()), executor_listener(exec), cc(cc), exec(exec) {}

    void coco_executor::log([[maybe_unused]] const std::string &msg) {}
    void coco_executor::read([[maybe_unused]] const std::string &script) {}
    void coco_executor::read([[maybe_unused]] const std::vector<std::string> &files) {}

    void coco_executor::state_changed() { cc.fire_state_changed(*this); }

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
    void coco_executor::flaw_state_changed([[maybe_unused]] const ratio::flaw &f) { cc.fire_flaw_state_changed(*this, f); }
    void coco_executor::flaw_cost_changed([[maybe_unused]] const ratio::flaw &f) { cc.fire_flaw_cost_changed(*this, f); }
    void coco_executor::flaw_position_changed([[maybe_unused]] const ratio::flaw &f) { cc.fire_flaw_position_changed(*this, f); }
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
    void coco_executor::resolver_state_changed([[maybe_unused]] const ratio::resolver &r) { cc.fire_resolver_state_changed(*this, r); }
    void coco_executor::current_resolver(const ratio::resolver &r)
    {
        c_resolver = &r;

        cc.fire_current_resolver(*this, r);
    }

    void coco_executor::causal_link_added([[maybe_unused]] const ratio::flaw &f, [[maybe_unused]] const ratio::resolver &r) { cc.fire_causal_link_added(*this, f, r); }

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
        std::unordered_map<const ratio::atom *, utils::rational> dsy;
        for (const auto &atm : atoms)
        {
            CLIPSValue res;
            Eval(cc.env, ("(starting " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " " + to_task(*atm) + ")").c_str(), &res);
            if (res.lexemeValue && std::strcmp(res.lexemeValue->contents, "FALSE") == 0)
                dsy[atm] = utils::rational::ONE;
            else if (res.multifieldValue)
                switch (res.multifieldValue->length)
                {
                case 1:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[atm] = utils::rational::ONE;
                    else if (res.multifieldValue[0].contents->integerValue)
                        dsy[atm] = utils::rational(res.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[atm] = utils::rational(res.multifieldValue[1].contents->integerValue->contents);
                    else if (res.multifieldValue[0].contents->integerValue && res.multifieldValue[1].contents->integerValue)
                        dsy[atm] = utils::rational(res.multifieldValue[0].contents->integerValue->contents, res.multifieldValue[1].contents->integerValue->contents);
                    break;
                case 3:
                    if (std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[atm] = utils::rational(res.multifieldValue[1].contents->integerValue->contents, res.multifieldValue[2].contents->integerValue->contents);
                    break;
                }
        }
        Run(cc.env, -1);
        exec.dont_start_yet(dsy);
    }
    void coco_executor::start(const std::unordered_set<ratio::atom *> &atoms)
    {
        for (const auto &atm : atoms)
            Eval(cc.env, ("(start " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " " + std::to_string(get_id(*atm)) + " " + to_task(*atm) + ")").c_str(), NULL);
        Run(cc.env, -1);
    }

    void coco_executor::ending(const std::unordered_set<ratio::atom *> &atoms)
    {
        std::unordered_map<const ratio::atom *, utils::rational> dey;
        for (const auto &atm : atoms)
        {
            CLIPSValue res;
            Eval(cc.env, ("(ending " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " " + std::to_string(get_id(*atm)) + ")").c_str(), &res);
            if (res.lexemeValue && std::strcmp(res.lexemeValue->contents, "FALSE") == 0)
                dey[atm] = utils::rational::ONE;
            else if (res.multifieldValue)
                switch (res.multifieldValue->length)
                {
                case 1:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[atm] = utils::rational::ONE;
                    else if (res.multifieldValue[0].contents->integerValue)
                        dey[atm] = utils::rational(res.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[atm] = utils::rational(res.multifieldValue[1].contents->integerValue->contents);
                    else if (res.multifieldValue[0].contents->integerValue && res.multifieldValue[1].contents->integerValue)
                        dey[atm] = utils::rational(res.multifieldValue[0].contents->integerValue->contents, res.multifieldValue[1].contents->integerValue->contents);
                    break;
                case 3:
                    if (std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[atm] = utils::rational(res.multifieldValue[1].contents->integerValue->contents, res.multifieldValue[2].contents->integerValue->contents);
                    break;
                }
        }
        Run(cc.env, -1);
        exec.dont_end_yet(dey);
    }
    void coco_executor::end(const std::unordered_set<ratio::atom *> &atoms)
    {
        for (const auto &atm : atoms)
            Eval(cc.env, ("(end " + std::to_string(reinterpret_cast<uintptr_t>(this)) + " " + std::to_string(get_id(*atm)) + ")").c_str(), NULL);
        Run(cc.env, -1);
    }

    std::string coco_executor::to_task(const ratio::atom &atm) const noexcept
    {
        std::string task_str = atm.get_type().get_name();
        std::string pars_str = " (create$ ";
        std::string vals_str = " (create$ ";

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

        return task_str + " " + pars_str + " " + vals_str;
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
