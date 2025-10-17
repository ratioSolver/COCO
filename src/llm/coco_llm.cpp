#include "coco_llm.hpp"
#include "coco.hpp"
#include "logging.hpp"
#include <cassert>

namespace coco
{
    coco_llm::coco_llm(coco &cc, std::string_view host, unsigned short port, std::string_view api_key) noexcept : coco_module(cc), client(host, port), async_client(), session(async_client.get_session(host, port)), api_key(api_key)
    {
        LOG_TRACE(llm_result_deftemplate);
        [[maybe_unused]] auto build_llm_result_dt_err = Build(get_env(), llm_result_deftemplate);
        assert(build_llm_result_dt_err == BE_NO_ERROR);

        [[maybe_unused]] auto understand_err = AddUDF(get_env(), "understand", "s", 1, 1, "s", understand_udf, "understand_udf", this);
        assert(understand_err == AUE_NO_ERROR);
        [[maybe_unused]] auto async_understand_err = AddUDF(get_env(), "async-understand", "v", 2, 2, "ys", async_understand_udf, "async_understand_udf", this);
        assert(async_understand_err == AUE_NO_ERROR);
    }

    std::string coco_llm::understand(std::string_view message) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        json::json j_prompt;
        j_prompt["model"] = LLM_MODEL;
        j_prompt["messages"] = std::vector<json::json>{{{"role", "user"}, {"content", message.data()}}};
        j_prompt["stream"] = false;

        auto res = client.post("/" LLM_PROVIDER "/v3/openai/chat/completions", std::move(j_prompt), {{"Content-Type", "application/json"}, {"Authorization", std::string("Bearer ") + api_key}});
        if (!res || res->get_status_code() != network::ok)
        {
            LOG_ERR("Failed to understand..");
            LOG_ERR(*res);
            return {};
        }
        auto llm_res = static_cast<network::json_response &>(*res).get_body();
        LOG_TRACE("Response:\n"
                  << llm_res);
        return llm_res["choices"][0]["message"]["content"].get<std::string>();
    }

    void coco_llm::async_understand(item &item, std::string_view message, bool infere) noexcept
    {
        json::json j_prompt;
        j_prompt["model"] = LLM_MODEL;
        j_prompt["messages"] = std::vector<json::json>{{{"role", "user"}, {"content", message.data()}}};
        j_prompt["stream"] = true;

        session->post("/" LLM_PROVIDER "/v3/openai/chat/completions", std::move(j_prompt), [this, &item, infere](const network::response &res)
                      {
                          if (res.get_status_code() != network::ok)
                          {
                              LOG_ERR("Failed to understand..");
                              LOG_ERR(res);
                              return;
                          }
                          auto s_res = static_cast<const network::string_response &>(res).get_body();
                          LOG_TRACE("Response:\n"
                                    << s_res);

                          std::lock_guard<std::recursive_mutex> _(get_mtx());
                          FactBuilder *item_fact_builder = CreateFactBuilder(get_env(), "llm-result");
                          FBPutSlotSymbol(item_fact_builder, "item_id", item.get_id().c_str());
                          FBPutSlotString(item_fact_builder, "result", s_res.c_str());
                          [[maybe_unused]] auto item_fact = FBAssert(item_fact_builder);
                          [[maybe_unused]] auto fb_err = FBError(get_env());
                          assert(fb_err == FBE_NO_ERROR);
                          assert(item_fact);
                          LOG_TRACE(to_string(item_fact));
                          FBDispose(item_fact_builder);

                          if (infere)
                              Run(get_env(), -1); },
                      {{"Content-Type", "application/json"}, {"Authorization", std::string("Bearer ") + api_key}});
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
