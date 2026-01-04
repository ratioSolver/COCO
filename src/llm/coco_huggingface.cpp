#include "coco_huggingface.hpp"
#include "logging.hpp"

namespace coco
{
    coco_huggingface::coco_huggingface(coco &cc, std::string_view host, unsigned short port, std::string_view api_key, std::string_view provider, std::string_view model) noexcept : coco_llm(cc), client(host, port), async_client(), session(async_client.get_session(host, port)), api_key(api_key), provider(provider), model(model) {}

    std::string coco_huggingface::understand(std::string_view message) noexcept
    {
        std::lock_guard<std::recursive_mutex> _(get_mtx());
        json::json j_prompt;
        j_prompt["model"] = model;
        j_prompt["messages"] = std::vector<json::json>{{{"role", "user"}, {"content", message.data()}}};
        j_prompt["stream"] = false;

        auto res = client.post("/" + provider + "/v3/openai/chat/completions", std::move(j_prompt), {{"Content-Type", "application/json"}, {"Authorization", std::string("Bearer ") + api_key}});
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

    void coco_huggingface::async_understand(item &item, std::string_view message, bool infere) noexcept
    {
        json::json j_prompt;
        j_prompt["model"] = model;
        j_prompt["messages"] = std::vector<json::json>{{{"role", "user"}, {"content", message.data()}}};
        j_prompt["stream"] = true;

        session->post("/" + provider + "/v3/openai/chat/completions", std::move(j_prompt), [this, &item, infere](const network::response &res)
                      {
                          if (res.get_status_code() != network::ok)
                          {
                              LOG_ERR("Failed to understand..");
                              LOG_ERR(res);
                              return;
                          }
                          auto llm_res = static_cast<const network::json_response &>(res).get_body();
                          LOG_TRACE("Response:\n"
                                    << llm_res.dump());

                          std::lock_guard<std::recursive_mutex> _(get_mtx());
                          FactBuilder *item_fact_builder = CreateFactBuilder(get_env(), "llm-result");
                          FBPutSlotSymbol(item_fact_builder, "item_id", item.get_id().c_str());
                          FBPutSlotString(item_fact_builder, "result", llm_res["choices"][0]["message"]["content"].get<std::string>().c_str());
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
} // namespace coco
