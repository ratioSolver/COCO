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
    void created_intent([[maybe_unused]] const intent &i) override;
    void created_entity([[maybe_unused]] const entity &e) override;
    void created_slot([[maybe_unused]] const slot &s) override;

  private:
    utils::u_ptr<network::response> get_intents(const network::request &);
    utils::u_ptr<network::response> create_intent(const network::request &req);
    utils::u_ptr<network::response> get_entities(const network::request &);
    utils::u_ptr<network::response> create_entity(const network::request &req);

  private:
    coco_llm &llm; // The LLM module
  };
} // namespace coco
