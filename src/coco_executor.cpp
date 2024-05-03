#include "coco_executor.hpp"
#include "coco_core.hpp"

namespace coco
{
    coco_solver::coco_solver(coco_executor &exec, const std::string &name) : ratio::solver(name), exec(exec) {}

    void coco_solver::flaw_created(const ratio::flaw &f) { exec.get_core().flaw_created(exec, f); }
    void coco_solver::flaw_state_changed(const ratio::flaw &f) { exec.get_core().flaw_state_changed(exec, f); }
    void coco_solver::flaw_cost_changed(const ratio::flaw &f) { exec.get_core().flaw_cost_changed(exec, f); }
    void coco_solver::flaw_position_changed(const ratio::flaw &f) { exec.get_core().flaw_position_changed(exec, f); }
    void coco_solver::current_flaw(const ratio::flaw &f) { exec.get_core().current_flaw(exec, f); }

    void coco_solver::resolver_created(const ratio::resolver &r) { exec.get_core().resolver_created(exec, r); }
    void coco_solver::resolver_state_changed(const ratio::resolver &r) { exec.get_core().resolver_state_changed(exec, r); }
    void coco_solver::current_resolver(const ratio::resolver &r) { exec.get_core().current_resolver(exec, r); }

    void coco_solver::causal_link_added(const ratio::flaw &f, const ratio::resolver &r) { exec.get_core().causal_link_added(exec, f, r); }

    coco_executor::coco_executor(coco_core &cc, const std::string &name, const utils::rational &units_per_tick) : executor(std::make_shared<coco_solver>(*this, name), units_per_tick), cc(cc) {}
} // namespace coco