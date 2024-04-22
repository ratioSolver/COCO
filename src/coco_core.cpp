#include <cassert>
#include "coco_core.hpp"

namespace coco
{
    coco_core::coco_core(coco_db &db) : db(db), env(CreateEnvironment())
    {
        assert(env != nullptr);
    }
} // namespace coco