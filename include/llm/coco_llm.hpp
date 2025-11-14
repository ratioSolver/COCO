#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"
#include "async_client.hpp"

namespace coco
{
  constexpr const char *llm_result_deftemplate = "(deftemplate llm-result (slot item_id (type SYMBOL)) (slot result (type STRING)))";

  [[nodiscard]] inline std::string default_llm_host() noexcept
  {
    const char *host = std::getenv("LLM_HOST");
    if (host)
      return host;
    return "api.coco-llm.com";
  }

  [[nodiscard]] inline unsigned short default_llm_port() noexcept
  {
    const char *port = std::getenv("LLM_PORT");
    if (port)
      return static_cast<unsigned short>(std::stoi(port));
    return 443;
  }

  [[nodiscard]] inline std::string default_llm_api_key() noexcept
  {
    std::ifstream file("run/secrets/llm_api_key");
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }

  class coco_llm final : public coco_module
  {
  public:
    coco_llm(coco &cc, std::string_view host = default_llm_host(), unsigned short port = default_llm_port(), std::string_view api_key = default_llm_api_key()) noexcept;

    std::string understand(std::string_view message) noexcept;

    void async_understand(item &item, std::string_view message, bool infere = true) noexcept;

    friend void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void async_understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    network::ssl_client client;                            // The client used to communicate with the LLM server
    network::ssl_async_client async_client;                // The asynchronous client used to communicate with the LLM server
    std::shared_ptr<network::client_session_base> session; // The client session used for asynchronous requests
    const std::string api_key;                             // The API key used to authenticate with the LLM server
  };

  void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
  void async_understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
