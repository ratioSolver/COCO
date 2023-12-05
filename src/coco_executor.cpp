#include "coco_executor.h"
#include "coco_core.h"
#include "coco_db.h"
#include <cstring>

namespace coco
{
    std::pair<Multifield *, Multifield *> to_task(Environment *env, const ratio::atom &atm) noexcept
    {
        std::string task_str = atm.get_type().get_name();

        auto pars = CreateMultifieldBuilder(env, atm.get_vars().size());
        auto vals = CreateMultifieldBuilder(env, atm.get_vars().size());
        auto &slv = static_cast<const ratio::solver &>(atm.get_type().get_core());

        for (const auto &[var_name, var] : atm.get_vars())
            if (!static_cast<const riddle::predicate &>(atm.get_type()).get_field(var_name).is_synthetic())
            {
                MBAppendSymbol(pars, var_name.c_str());
                if (&var->get_type() == &slv.get_bool_type())
                {
                    switch (slv.get_sat_core().value(static_cast<const ratio::bool_item &>(*var).get_lit()))
                    {
                    case utils::True:
                        MBAppendSymbol(vals, "TRUE");
                        break;
                    case utils::False:
                        MBAppendSymbol(vals, "FALSE");
                        break;
                    case utils::Undefined:
                        MBAppendSymbol(vals, "UNDEFINED");
                        break;
                    }
                }
                else if (&var->get_type() == &slv.get_real_type())
                {
                    auto val = slv.get_lra_theory().value(static_cast<const ratio::arith_item &>(*var).get_lin());
                    MBAppendFloat(vals, static_cast<double>(val.get_rational().numerator()) / val.get_rational().denominator());
                }
                else if (&var->get_type() == &slv.get_time_type())
                {
                    const auto [lb, ub] = slv.get_rdl_theory().bounds(static_cast<const ratio::arith_item &>(*var).get_lin());
                    MBAppendFloat(vals, static_cast<double>(lb.get_rational().numerator()) / lb.get_rational().denominator());
                }
                else if (&var->get_type() == &slv.get_string_type())
                    MBAppendString(vals, static_cast<const ratio::string_item &>(*var).get_string().c_str());
                else if (auto ev = dynamic_cast<const ratio::enum_item *>(&*var))
                {
                    const auto e_vals = slv.get_ov_theory().value(ev->get_var());
                    if (e_vals.size() == 1)
                        MBAppendSymbol(vals, slv.guess_name(dynamic_cast<riddle::item &>(**e_vals.begin())).c_str());
                    else
                    {
                        auto e_vals_mf = CreateMultifieldBuilder(env, e_vals.size());
                        for (const auto &e_val : e_vals)
                            MBAppendSymbol(e_vals_mf, slv.guess_name(dynamic_cast<riddle::item &>(*e_val)).c_str());
                        MBAppendMultifield(vals, MBCreate(e_vals_mf));
                    }
                }
                else
                    MBAppendSymbol(vals, slv.guess_name(*var).c_str());
            }

        return {MBCreate(pars), MBCreate(vals)};
    }

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
            auto starting_call = CreateFunctionCallBuilder(cc.env, 4);
            FCBAppendInteger(starting_call, reinterpret_cast<uintptr_t>(this)); // the solver pointer
            FCBAppendSymbol(starting_call, atm->get_type().get_name().c_str()); // the task type
            auto task = to_task(cc.env, *atm);                                  // we create the multifields for the task's parameters and values
            FCBAppendMultifield(starting_call, task.first);                     // the task parameters
            FCBAppendMultifield(starting_call, task.second);                    // the task values
            FCBCall(starting_call, "starting", &res);
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
        {
            auto start_call = CreateFunctionCallBuilder(cc.env, 5);
            FCBAppendInteger(start_call, reinterpret_cast<uintptr_t>(this)); // the solver pointer
            FCBAppendInteger(start_call, get_id(*atm));                      // the task id
            FCBAppendSymbol(start_call, atm->get_type().get_name().c_str()); // the task type
            auto task = to_task(cc.env, *atm);                               // we create the multifields for the task's parameters and values
            FCBAppendMultifield(start_call, task.first);                     // the task parameters
            FCBAppendMultifield(start_call, task.second);                    // the task values
            FCBCall(start_call, "start", NULL);
        }
        Run(cc.env, -1);
        cc.fire_start(*this, atoms);
    }

    void coco_executor::ending(const std::unordered_set<ratio::atom *> &atoms)
    {
        std::unordered_map<const ratio::atom *, utils::rational> dey;
        for (const auto &atm : atoms)
        {
            CLIPSValue res;
            auto ending_call = CreateFunctionCallBuilder(cc.env, 2);
            FCBAppendInteger(ending_call, reinterpret_cast<uintptr_t>(this)); // the solver pointer
            FCBAppendInteger(ending_call, get_id(*atm));                      // the task id
            FCBCall(ending_call, "ending", &res);
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
        {
            auto end_call = CreateFunctionCallBuilder(cc.env, 2);
            FCBAppendInteger(end_call, reinterpret_cast<uintptr_t>(this)); // the solver pointer
            FCBAppendInteger(end_call, get_id(*atm));                      // the task id
            FCBCall(end_call, "end", NULL);
        }
        Run(cc.env, -1);
        cc.fire_end(*this, atoms);
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
