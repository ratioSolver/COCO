#pragma once

#include "coco_llm.hpp"

namespace coco
{
  [[nodiscard]] inline std::string default_llm_host() noexcept
  {
    const char *host = std::getenv("LLM_HOST");
    if (host)
      return host;
    return "router.huggingface.co";
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

  [[nodiscard]] inline std::string default_llm_provider() noexcept
  {
    const char *provider = std::getenv("LLM_PROVIDER");
    if (provider)
      return provider;
    return "novita";
  }

  [[nodiscard]] inline std::string default_llm_model() noexcept
  {
    const char *model = std::getenv("LLM_MODEL");
    if (model)
      return model;
    return "meta-llama/llama-3.3-70b-instruct";
  }

  class coco_huggingface final : public coco_llm
  {
  public:
    coco_huggingface(coco &cc, std::string_view host = default_llm_host(), unsigned short port = default_llm_port(), std::string_view api_key = default_llm_api_key(), std::string_view provider = default_llm_provider(), std::string_view model = default_llm_model()) noexcept;

    std::string understand(std::string_view message) noexcept override;

    void async_understand(item &item, std::string_view message, bool infere = true) noexcept override;

  private:
    network::ssl_client client;                            // The client used to communicate with the LLM server
    network::ssl_async_client async_client;                // The asynchronous client used to communicate with the LLM server
    std::shared_ptr<network::client_session_base> session; // The client session used for asynchronous requests
    const std::string api_key;                             // The API key used to authenticate with the LLM server
    const std::string provider;                            // The provider used to communicate with the LLM server
    const std::string model;                               // The model used to communicate with the LLM server
  };
} // namespace coco
