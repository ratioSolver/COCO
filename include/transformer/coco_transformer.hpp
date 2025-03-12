#pragma once

#include "coco.hpp"
#include "client.hpp"

namespace coco
{
  constexpr const char *intent_deftemplate = "(deftemplate intent (slot name (type SYMBOL)) (slot confidence (type FLOAT)) (multislot entities (type SYMBOL)) (multislot values (type STRING)) (multislot confidences (type FLOAT)))";

  class transformer : public listener
  {
  public:
    transformer(coco &cc, std::string_view host = TRANSFORMER_HOST, unsigned short port = TRANSFORMER_PORT) noexcept;

  private:
    friend void understand(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void trigger_intent(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void compute_response(Environment *env, UDFContext *udfc, UDFValue *out);

  private:
    network::client client; // the transformer client..
  };

  void understand(Environment *env, UDFContext *udfc, UDFValue *out);
  void trigger_intent(Environment *env, UDFContext *udfc, UDFValue *out);
  void compute_response(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
