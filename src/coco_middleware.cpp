#include "coco_middleware.h"
#include "coco.h"

namespace coco
{
    coco_middleware::coco_middleware(coco &cc) : cc(cc) {}

    void coco_middleware::message_arrived(const std::string &topic, json::json &msg) { cc.message_arrived(topic, msg); }
} // namespace coco
