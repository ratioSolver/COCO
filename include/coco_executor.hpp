#pragma once

#include "executor.hpp"
#include "clips.h"

namespace coco
{
  class coco_core;

  class coco_executor final : public ratio::executor::executor
  {
  public:
    coco_executor(coco_core &cc, std::shared_ptr<ratio::solver> slv = std::make_shared<ratio::solver>(), const utils::rational &units_per_tick = utils::rational::one);

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
   * @brief Gets the unique identifier of the given solver.
   *
   * @param exec the solver to get the unique identifier of.
   * @return uintptr_t the unique identifier of the given solver.
   */
  inline uintptr_t get_id(const coco_executor &exec) { return reinterpret_cast<uintptr_t>(&exec); }
} // namespace coco