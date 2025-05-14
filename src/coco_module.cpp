#include "coco_module.hpp"
#include "coco.hpp"

namespace coco
{
    coco_module::coco_module(coco &cc) noexcept : cc(cc) {}

    std::recursive_mutex &coco_module::get_mtx() const { return cc.mtx; }
    Environment *coco_module::get_env() const { return cc.env; }

    std::string coco_module::to_string(Fact *f, std::size_t buff_size) const noexcept { return cc.to_string(f, buff_size); }
} // namespace coco
