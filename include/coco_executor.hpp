#pragma once

#include "executor.hpp"
#include "clips.h"

namespace coco
{
  class coco_core;
  class coco_executor;

  class coco_solver final : public ratio::solver
  {
  public:
    coco_solver(coco_executor &exec, const std::string &name);

  private:
    void state_changed() override;

    void flaw_created(const ratio::flaw &f) override;
    void flaw_state_changed(const ratio::flaw &f) override;
    void flaw_cost_changed(const ratio::flaw &f) override;
    void flaw_position_changed(const ratio::flaw &f) override;
    void current_flaw(const ratio::flaw &f) override;

    void resolver_created(const ratio::resolver &r) override;
    void resolver_state_changed(const ratio::resolver &r) override;
    void current_resolver(const ratio::resolver &r) override;

    void causal_link_added(const ratio::flaw &f, const ratio::resolver &r) override;

  private:
    coco_executor &exec;
  };

  class coco_executor final : public ratio::executor::executor
  {
  public:
    coco_executor(coco_core &cc, const std::string &name, const utils::rational &units_per_tick = utils::rational::one);

    coco_core &get_core() const { return cc; }

  private:
    void executor_state_changed(ratio::executor::executor_state state) override;
    void tick(const utils::rational &time) override;

    void starting(const std::vector<std::reference_wrapper<const ratio::atom>> &atms) override;
    void start(const std::vector<std::reference_wrapper<const ratio::atom>> &atms) override;

    void ending(const std::vector<std::reference_wrapper<const ratio::atom>> &atms) override;
    void end(const std::vector<std::reference_wrapper<const ratio::atom>> &atms) override;

  private:
    coco_core &cc;
  };

  /**
   * @brief Gets the unique identifier of the given executor.
   *
   * @param exec the executor to get the unique identifier of.
   * @return uintptr_t the unique identifier of the given executor.
   */
  inline uintptr_t get_id(const coco_executor &exec) { return reinterpret_cast<uintptr_t>(&exec); }
} // namespace coco