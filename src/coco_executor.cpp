#include "coco_executor.hpp"
#include "coco_core.hpp"
#include <cstring>

namespace coco
{
    coco_solver::coco_solver(coco_executor &exec, const std::string &name) : ratio::solver(name), exec(exec) {}

    void coco_solver::state_changed() { exec.get_core().state_changed(exec); }

    void coco_solver::flaw_created(const ratio::flaw &f) { exec.get_core().flaw_created(exec, f); }
    void coco_solver::flaw_state_changed(const ratio::flaw &f) { exec.get_core().flaw_state_changed(exec, f); }
    void coco_solver::flaw_cost_changed(const ratio::flaw &f) { exec.get_core().flaw_cost_changed(exec, f); }
    void coco_solver::flaw_position_changed(const ratio::flaw &f) { exec.get_core().flaw_position_changed(exec, f); }
    void coco_solver::current_flaw(const ratio::flaw &f) { exec.get_core().current_flaw(exec, f); }

    void coco_solver::resolver_created(const ratio::resolver &r) { exec.get_core().resolver_created(exec, r); }
    void coco_solver::resolver_state_changed(const ratio::resolver &r) { exec.get_core().resolver_state_changed(exec, r); }
    void coco_solver::current_resolver(const ratio::resolver &r) { exec.get_core().current_resolver(exec, r); }

    void coco_solver::causal_link_added(const ratio::flaw &f, const ratio::resolver &r) { exec.get_core().causal_link_added(exec, f, r); }

    coco_executor::coco_executor(coco_core &cc, const std::string &name, const utils::rational &units_per_tick) : executor(std::make_shared<coco_solver>(*this, name), units_per_tick), cc(cc)
    {
        FactBuilder *solver_fact_builder = CreateFactBuilder(cc.env, "solver");
        FBPutSlotInteger(solver_fact_builder, "id", get_id(*this));
        FBPutSlotSymbol(solver_fact_builder, "purpose", name.c_str());
        FBPutSlotSymbol(solver_fact_builder, "state", to_string(get_state()).c_str());
        solver_fact = FBAssert(solver_fact_builder);
        FBDispose(solver_fact_builder);
    }
    coco_executor::~coco_executor()
    {
        Retract(solver_fact);
    }

    std::pair<Multifield *, Multifield *> to_task(Environment *env, const ratio::atom &atm) noexcept
    {
        std::string task_str = atm.get_type().get_name();

        auto pars = CreateMultifieldBuilder(env, atm.get_items().size());
        auto vals = CreateMultifieldBuilder(env, atm.get_items().size());
        auto &slv = static_cast<const ratio::solver &>(atm.get_type().get_scope().get_core());

        for (const auto &[var_name, var] : atm.get_items())
            if (!static_cast<const riddle::predicate &>(atm.get_type()).get_field(var_name).value().get().is_synthetic())
            {
                MBAppendSymbol(pars, var_name.c_str());
                if (&var->get_type() == &slv.get_bool_type())
                {
                    switch (slv.get_sat().value(static_cast<const riddle::bool_item &>(*var).get_value()))
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
                    auto val = slv.get_lra_theory().value(static_cast<const riddle::arith_item &>(*var).get_value());
                    MBAppendFloat(vals, static_cast<double>(val.get_rational().numerator()) / val.get_rational().denominator());
                }
                else if (&var->get_type() == &slv.get_time_type())
                {
                    const auto [lb, ub] = slv.get_rdl_theory().bounds(static_cast<const riddle::arith_item &>(*var).get_value());
                    MBAppendFloat(vals, static_cast<double>(lb.get_rational().numerator()) / lb.get_rational().denominator());
                }
                else if (&var->get_type() == &slv.get_string_type())
                    MBAppendString(vals, static_cast<const riddle::string_item &>(*var).get_value().c_str());
                else if (auto ev = dynamic_cast<const riddle::enum_item *>(&*var))
                {
                    const auto e_vals = slv.get_ov_theory().domain(ev->get_value());
                    if (e_vals.size() == 1)
                        MBAppendSymbol(vals, slv.guess_name(dynamic_cast<riddle::item &>(e_vals.begin()->get())).c_str());
                    else
                    {
                        auto e_vals_mf = CreateMultifieldBuilder(env, e_vals.size());
                        for (const auto &e_val : e_vals)
                            MBAppendSymbol(e_vals_mf, slv.guess_name(dynamic_cast<riddle::item &>(e_val.get())).c_str());
                        MBAppendMultifield(vals, MBCreate(e_vals_mf));
                    }
                }
                else
                    MBAppendSymbol(vals, slv.guess_name(*var).c_str());
            }

        return {MBCreate(pars), MBCreate(vals)};
    }

    void coco_executor::executor_state_changed(ratio::executor::executor_state state)
    {
        FactModifier *solver_fact_modifier = CreateFactModifier(cc.env, solver_fact);
        FMPutSlotSymbol(solver_fact_modifier, "state", to_string(state).c_str());
        solver_fact = FMModify(solver_fact_modifier);
        FMDispose(solver_fact_modifier);
        cc.executor_state_changed(*this, state);
    }
    void coco_executor::tick(const utils::rational &time) { cc.tick(*this, time); }

    void coco_executor::starting(const std::vector<std::reference_wrapper<const ratio::atom>> &atms)
    {
        std::unordered_map<const ratio::atom *, utils::rational> dsy; // we compute the delays for don't start yet atoms
        for (const auto &atm : atms)
        {
            FunctionCallBuilder *starting_call_builder = CreateFunctionCallBuilder(cc.env, 4);
            FCBAppendInteger(starting_call_builder, get_id(*this));                          // the solver pointer
            FCBAppendString(starting_call_builder, atm.get().get_type().get_name().c_str()); // the task name
            auto task = to_task(cc.env, atm.get());                                          // we create the multifields for the task's parameters and values
            FCBAppendMultifield(starting_call_builder, task.first);                          // the task's parameters
            FCBAppendMultifield(starting_call_builder, task.second);                         // the task's values
            CLIPSValue res;
            FCBCall(starting_call_builder, "starting", &res); // we call the function
            FCBDispose(starting_call_builder);
            if (res.lexemeValue && std::strcmp(res.lexemeValue->contents, "FALSE") == 0)
                dsy[&atm.get()] = utils::rational::one;
            else if (res.multifieldValue)
                switch (res.multifieldValue->length)
                {
                case 1:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[&atm.get()] = utils::rational::one;
                    else if (res.multifieldValue[0].contents->integerValue)
                        dsy[&atm.get()] = utils::rational(res.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[&atm.get()] = utils::rational(res.multifieldValue[1].contents->integerValue->contents);
                    else if (res.multifieldValue[0].contents->integerValue && res.multifieldValue[1].contents->integerValue)
                        dsy[&atm.get()] = utils::rational(res.multifieldValue[0].contents->integerValue->contents, res.multifieldValue[1].contents->integerValue->contents);
                    break;
                case 3:
                    if (std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dsy[&atm.get()] = utils::rational(res.multifieldValue[1].contents->integerValue->contents, res.multifieldValue[2].contents->integerValue->contents);
                    break;
                }
        }
        dont_start_yet(dsy);
        cc.starting(*this, atms);
    }
    void coco_executor::start(const std::vector<std::reference_wrapper<const ratio::atom>> &atms)
    {
        FactBuilder *task_fact_builder = CreateFactBuilder(cc.env, "task");
        FBPutSlotInteger(task_fact_builder, "solver_id", get_id(*this));
        for (const auto &atm : atms)
        {
            auto task = to_task(cc.env, atm.get());
            FBPutSlotInteger(task_fact_builder, "id", get_id(atm.get()));
            FBPutSlotSymbol(task_fact_builder, "fact_type", atm.get().get_type().get_name().c_str());
            FBPutSlotMultifield(task_fact_builder, "pars", task.first);
            FBPutSlotMultifield(task_fact_builder, "vals", task.second);
            task_facts.emplace(&atm.get(), FBAssert(task_fact_builder));
        }
        FBDispose(task_fact_builder);
        cc.start(*this, atms);
    }

    void coco_executor::ending(const std::vector<std::reference_wrapper<const ratio::atom>> &atms)
    {
        std::unordered_map<const ratio::atom *, utils::rational> dey; // we compute the delays for don't end yet atoms
        for (const auto &atm : atms)
        {
            FunctionCallBuilder *ending_call_builder = CreateFunctionCallBuilder(cc.env, 4);
            FCBAppendInteger(ending_call_builder, get_id(*this));                          // the solver pointer
            FCBAppendString(ending_call_builder, atm.get().get_type().get_name().c_str()); // the task name
            auto task = to_task(cc.env, atm.get());                                        // we create the multifields for the task's parameters and values
            FCBAppendMultifield(ending_call_builder, task.first);                          // the task's parameters
            FCBAppendMultifield(ending_call_builder, task.second);                         // the task's values
            CLIPSValue res;
            FCBCall(ending_call_builder, "ending", &res); // we call the function
            FCBDispose(ending_call_builder);
            if (res.lexemeValue && std::strcmp(res.lexemeValue->contents, "FALSE") == 0)
                dey[&atm.get()] = utils::rational::one;
            else if (res.multifieldValue)
                switch (res.multifieldValue->length)
                {
                case 1:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[&atm.get()] = utils::rational::one;
                    else if (res.multifieldValue[0].contents->integerValue)
                        dey[&atm.get()] = utils::rational(res.multifieldValue[0].contents->integerValue->contents);
                    break;
                case 2:
                    if (res.multifieldValue[0].contents->lexemeValue && std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[&atm.get()] = utils::rational(res.multifieldValue[1].contents->integerValue->contents);
                    else if (res.multifieldValue[0].contents->integerValue && res.multifieldValue[1].contents->integerValue)
                        dey[&atm.get()] = utils::rational(res.multifieldValue[0].contents->integerValue->contents, res.multifieldValue[1].contents->integerValue->contents);
                    break;
                case 3:
                    if (std::strcmp(res.multifieldValue[0].contents->lexemeValue->contents, "FALSE") == 0)
                        dey[&atm.get()] = utils::rational(res.multifieldValue[1].contents->integerValue->contents, res.multifieldValue[2].contents->integerValue->contents);
                    break;
                }
        }
        dont_end_yet(dey);
        cc.ending(*this, atms);
    }
    void coco_executor::end(const std::vector<std::reference_wrapper<const ratio::atom>> &atms)
    {
        for (const auto &atm : atms)
        {
            Retract(task_facts.at(&atm.get()));
            task_facts.erase(&atm.get());
        }
        cc.end(*this, atms);
    }
} // namespace coco