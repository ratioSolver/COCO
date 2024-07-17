#include "coco_rule.hpp"
#include "coco_core.hpp"

namespace coco
{
    rule::rule(coco_core &cc, const std::string &id, const std::string &name, const std::string &content, bool reactive) noexcept : cc(cc), id(id), name(name), content(content), reactive(reactive)
    {
        if (reactive)
            Build(cc.env, content.c_str());
    }
} // namespace coco
