#include "coco_rule.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    reactive_rule::reactive_rule(coco &cc, std::string_view name, std::string_view content) noexcept : cc(cc), name(name), content(content)
    {
        LOG_TRACE(content);
        [[maybe_unused]] auto build_rl_err = Build(cc.env, content.data());
        assert(build_rl_err == BE_NO_ERROR);
    }
    reactive_rule::~reactive_rule()
    {
        auto defrule = FindDefrule(cc.env, name.c_str());
        assert(defrule);
        assert(DefruleIsDeletable(defrule));
        [[maybe_unused]] auto undef_rl = Undefrule(defrule, cc.env);
        assert(undef_rl);
    }

    json::json reactive_rule::to_json() const noexcept { return {{"content", content.c_str()}}; }

    deliberative_rule::deliberative_rule(coco &cc, std::string_view name, std::string_view content) noexcept : cc(cc), name(name), content(content) {}

    json::json deliberative_rule::to_json() const noexcept { return {{"content", content.c_str()}}; }
} // namespace coco
