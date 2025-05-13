#include "coco_module.hpp"
#include "coco.hpp"

namespace coco
{
    std::recursive_mutex &coco_module::get_mtx() const { return cc.mtx; }
    Environment *coco_module::get_env() const { return cc.env; }
} // namespace coco
