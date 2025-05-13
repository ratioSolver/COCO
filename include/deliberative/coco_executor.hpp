#pragma once

#if defined(SEMITONE)
#include "stexecutor.hpp"
#elif defined(MathSAT)
#include "msatexecutor.hpp"
#elif defined(Z3)
#include "z3executor.hpp"
#endif

namespace coco
{
  class coco_deliberative;

  class coco_executor : public ratio::executor::executor
  {
  public:
    coco_executor(coco_deliberative &cd, std::string_view name = "oRatio", const utils::rational &units_per_tick = utils::rational::one) noexcept;

#ifdef BUILD_LISTENERS
  private:
    void state_changed() override;

    void flaw_created(const ratio::flaw &) override;
    void flaw_state_changed(const ratio::flaw &) override;
    void flaw_cost_changed(const ratio::flaw &) override;
    void flaw_position_changed(const ratio::flaw &) override;
    void current_flaw(std::optional<utils::ref_wrapper<ratio::flaw>>) override;
    void resolver_created(const ratio::resolver &) override;
    void resolver_state_changed(const ratio::resolver &) override;
    void current_resolver(std::optional<utils::ref_wrapper<ratio::resolver>>) override;
    void causal_link_added(const ratio::flaw &, const ratio::resolver &) override;

    void executor_state_changed(ratio::executor::executor_state) override;
    void tick(const utils::rational &) override;
    void starting(const std::vector<utils::ref_wrapper<riddle::atom_term>> &) override;
    void start(const std::vector<utils::ref_wrapper<riddle::atom_term>> &) override;
    void ending(const std::vector<utils::ref_wrapper<riddle::atom_term>> &) override;
    void end(const std::vector<utils::ref_wrapper<riddle::atom_term>> &) override;
#endif

  private:
    coco_deliberative &cd; // reference to the coco object
  };
} // namespace coco
