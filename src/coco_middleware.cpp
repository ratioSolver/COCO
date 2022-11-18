#include "coco_middleware.h"
#include "coco.h"

namespace coco
{
    coco_middleware::coco_middleware(coco &cc) : cc(cc) {}

    void coco_middleware::message_arrived(const json::json &msg) { cc.message_arrived(msg); }
} // namespace coco
