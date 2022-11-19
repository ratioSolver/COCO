#pragma once

#include "json.h"

namespace coco
{
  class coco;

  class coco_middleware
  {
  public:
    coco_middleware(coco &cc);
    virtual ~coco_middleware() = default;

    virtual void connect() = 0;
    virtual void disconnect() = 0;

    virtual void subscribe(const std::string &topic, int qos = 0) = 0;
    virtual void publish(const std::string &topic, json::json &msg, int qos = 0, bool retained = false) = 0;

  protected:
    void message_arrived(json::json &msg);

  protected:
    coco &cc;
  };
} // namespace coco
