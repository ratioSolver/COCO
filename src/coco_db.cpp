#include "coco_db.hpp"

namespace coco
{
    coco_db::coco_db(json::json &&config) noexcept : config(std::move(config)) {}
} // namespace coco
