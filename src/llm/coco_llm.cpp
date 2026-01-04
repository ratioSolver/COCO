#include "coco_llm.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    coco_llm::coco_llm(coco &cc) noexcept : coco_module(cc)
    {
        LOG_TRACE(llm_result_deftemplate);
        [[maybe_unused]] auto build_llm_result_dt_err = Build(get_env(), llm_result_deftemplate);
        assert(build_llm_result_dt_err == BE_NO_ERROR);

        [[maybe_unused]] auto understand_err = AddUDF(get_env(), "understand", "s", 1, 1, "s", understand_udf, "understand_udf", this);
        assert(understand_err == AUE_NO_ERROR);
        [[maybe_unused]] auto async_understand_err = AddUDF(get_env(), "async-understand", "v", 2, 2, "ys", async_understand_udf, "async_understand_udf", this);
        assert(async_understand_err == AUE_NO_ERROR);
    }

    void understand_udf(Environment *env, UDFContext *udfc, UDFValue *res)
    {
        LOG_DEBUG("Understanding..");

        auto &llm = *reinterpret_cast<coco_llm *>(udfc->context);

        UDFValue message; // we get the message..
        if (!UDFFirstArgument(udfc, STRING_BIT, &message))
            return;

        res->lexemeValue = CreateString(env, llm.understand(message.lexemeValue->contents).c_str());
    }

    void async_understand_udf(Environment *, UDFContext *udfc, UDFValue *)
    {
        LOG_DEBUG("Async Understanding..");

        auto &llm = *reinterpret_cast<coco_llm *>(udfc->context);

        UDFValue item_id; // we get the item id..
        if (!UDFFirstArgument(udfc, SYMBOL_BIT, &item_id))
            return;
        UDFValue message; // we get the message..
        if (!UDFNextArgument(udfc, STRING_BIT, &message))
            return;

        auto &itm = llm.get_coco().get_item(item_id.lexemeValue->contents);
        llm.async_understand(itm, message.lexemeValue->contents, true);
    }
} // namespace coco
