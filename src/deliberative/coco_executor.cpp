#include "coco_executor.hpp"
#include "coco_deliberative.hpp"

namespace coco
{
    coco_executor::coco_executor(coco_deliberative &cd, std::string_view name, const utils::rational &units_per_tick) noexcept : ratio::executor::executor(name, units_per_tick), cd(cd) {}

#ifdef BUILD_LISTENERS
    void coco_executor::state_changed() { cd.state_changed(*this); }

    void coco_executor::flaw_created(const ratio::flaw &f) { cd.flaw_created(*this, f); }
    void coco_executor::flaw_state_changed(const ratio::flaw &f) { cd.flaw_state_changed(*this, f); }
    void coco_executor::flaw_cost_changed(const ratio::flaw &f) { cd.flaw_cost_changed(*this, f); }
    void coco_executor::flaw_position_changed(const ratio::flaw &f) { cd.flaw_position_changed(*this, f); }
    void coco_executor::current_flaw(std::optional<std::reference_wrapper<ratio::flaw>> f) { cd.current_flaw(*this, f); }
    void coco_executor::resolver_created(const ratio::resolver &r) { cd.resolver_created(*this, r); }
    void coco_executor::resolver_state_changed(const ratio::resolver &r) { cd.resolver_state_changed(*this, r); }
    void coco_executor::current_resolver(std::optional<std::reference_wrapper<ratio::resolver>> r) { cd.current_resolver(*this, r); }
    void coco_executor::causal_link_added(const ratio::flaw &f, const ratio::resolver &r) { cd.causal_link_added(*this, f, r); }

    void coco_executor::executor_state_changed(ratio::executor::executor_state state) { cd.executor_state_changed(*this, state); }
    void coco_executor::tick(const utils::rational &time) { cd.tick(*this, time); }
    void coco_executor::starting(const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) { cd.starting(*this, atms); }
    void coco_executor::start(const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) { cd.start(*this, atms); }
    void coco_executor::ending(const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) { cd.ending(*this, atms); }
    void coco_executor::end(const std::vector<std::reference_wrapper<riddle::atom_term>> &atms) { cd.end(*this, atms); }
#endif
} // namespace coco
