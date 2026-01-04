#pragma once

#include "coco_module.hpp"
#include "coco_item.hpp"
#include "client.hpp"
#include "async_client.hpp"

namespace coco
{
  constexpr const char *llm_result_deftemplate = "(deftemplate llm-result (slot item_id (type SYMBOL)) (slot result (type STRING)))";

  class coco_llm : public coco_module
  {
  public:
    coco_llm(coco &cc) noexcept;

    virtual std::string understand(std::string_view message) = 0;

    virtual void async_understand(item &item, std::string_view message, bool infere = true) = 0;

    friend void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
    friend void async_understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
  };

  void understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
  void async_understand_udf(Environment *env, UDFContext *udfc, UDFValue *out);
} // namespace coco
