#pragma once

#include "coco_server.hpp"
#include "coco_llm.hpp"

namespace coco
{
  class llm_server : public server_module, public llm_listener
  {
  public:
    llm_server(coco_server &srv, coco_llm &llm) noexcept;

  private:
    void created_intent(const intent &i) override;
    void created_entity(const entity &e) override;
    void created_slot(const slot &s) override;

  private:
    std::unique_ptr<network::response> get_intents(const network::request &);
    std::unique_ptr<network::response> create_intent(const network::request &req);
    std::unique_ptr<network::response> get_entities(const network::request &);
    std::unique_ptr<network::response> create_entity(const network::request &req);
    std::unique_ptr<network::response> get_slots(const network::request &);
    std::unique_ptr<network::response> create_slot(const network::request &req);

  private:
    coco_llm &llm; // The LLM module
  };
} // namespace coco
