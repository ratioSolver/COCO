#include "coco_executor.hpp"

namespace coco
{
    coco_executor::coco_executor(coco_core &cc, std::shared_ptr<ratio::solver> slv, const utils::rational &units_per_tick) : executor(slv, units_per_tick), cc(cc) {}
} // namespace coco