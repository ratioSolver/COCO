#include "coco_executor.hpp"

namespace coco
{
    coco_executor::coco_executor(coco &cc, const utils::rational &units_per_tick) noexcept : ratio::executor::executor(units_per_tick), cc(cc)
    {
    }
} // namespace coco