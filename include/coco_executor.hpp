#pragma once

#include "executor.hpp"
#include "clips.h"

namespace coco
{
  class coco_core;

  class coco_executor final : public ratio::executor::executor
  {
  public:
    coco_executor(coco_core &cc);

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
} // namespace coco