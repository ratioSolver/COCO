#pragma once

#include "coco_llm.hpp"

namespace coco
{
  [[nodiscard]] inline std::string default_llm_host() noexcept
  {
    const char *host = std::getenv("LLM_HOST");
    if (host)
      return host;
    return "localhost";
  }

  [[nodiscard]] inline unsigned short default_llm_port() noexcept
  {
    const char *port = std::getenv("LLM_PORT");
    if (port)
      return static_cast<unsigned short>(std::stoi(port));
    return 11434;
  }

  [[nodiscard]] inline std::string default_llm_model() noexcept
  {
    const char *model = std::getenv("LLM_MODEL");
    if (model)
      return model;
    return "llama3";
  }

  class coco_ollama final : public coco_llm
  {
  public:
    coco_ollama(coco &cc, std::string_view host = default_llm_host(), unsigned short port = default_llm_port(), std::string_view model = default_llm_model()) noexcept;

    std::string understand(std::string_view message) noexcept override;

    void async_understand(item &item, std::string_view message, bool infere = true) noexcept override;

  private:
    network::client client;                                // The client used to communicate with the LLM server
    network::async_client async_client;                    // The asynchronous client used to communicate with the LLM server
    std::shared_ptr<network::client_session_base> session; // The client session used for asynchronous requests
    const std::string model;                               // The model used to communicate with the LLM server
  };
} // namespace coco
