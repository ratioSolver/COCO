#include "coco_executor.hpp"
#include "coco.hpp"

namespace coco
{
    coco_executor::coco_executor(coco &cc, std::string_view name, const utils::rational &units_per_tick) noexcept : ratio::executor::executor(name, units_per_tick), cc(cc) {}

    void coco_executor::state_changed() { cc.state_changed(*this); }

    void coco_executor::flaw_created(const ratio::flaw &f) { cc.flaw_created(*this, f); }
    void coco_executor::flaw_state_changed(const ratio::flaw &f) { cc.flaw_state_changed(*this, f); }
    void coco_executor::flaw_cost_changed(const ratio::flaw &f) { cc.flaw_cost_changed(*this, f); }
    void coco_executor::flaw_position_changed(const ratio::flaw &f) { cc.flaw_position_changed(*this, f); }
    void coco_executor::current_flaw(std::optional<utils::ref_wrapper<ratio::flaw>> f) { cc.current_flaw(*this, f); }
    void coco_executor::resolver_created(const ratio::resolver &r) { cc.resolver_created(*this, r); }
    void coco_executor::resolver_state_changed(const ratio::resolver &r) { cc.resolver_state_changed(*this, r); }
    void coco_executor::current_resolver(std::optional<utils::ref_wrapper<ratio::resolver>> r) { cc.current_resolver(*this, r); }
    void coco_executor::causal_link_added(const ratio::flaw &f, const ratio::resolver &r) { cc.causal_link_added(*this, f, r); }

    void coco_executor::executor_state_changed(ratio::executor::executor_state state) { cc.executor_state_changed(*this, state); }
    void coco_executor::tick(const utils::rational &time) { cc.tick(*this, time); }
    void coco_executor::starting(const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) { cc.starting(*this, atms); }
    void coco_executor::start(const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) { cc.start(*this, atms); }
    void coco_executor::ending(const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) { cc.ending(*this, atms); }
    void coco_executor::end(const std::vector<utils::ref_wrapper<riddle::atom_term>> &atms) { cc.end(*this, atms); }
} // namespace coco