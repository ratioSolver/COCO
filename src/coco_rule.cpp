#include "coco_rule.hpp"
#include "coco.hpp"
#include "logging.hpp"

namespace coco
{
    reactive_rule::reactive_rule(coco &cc, std::string_view name, std::string_view content) noexcept : cc(cc), name(name), content(content)
    {
        LOG_TRACE(content);
        Build(cc.env, content.data());
    }

    deliberative_rule::deliberative_rule(coco &cc, std::string_view name, std::string_view content) noexcept : cc(cc), name(name), content(content) {}
} // namespace coco
