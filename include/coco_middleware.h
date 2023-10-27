#pragma once

#include "json.h"
#include <memory>

namespace coco
{
  class coco_core;

  class coco_middleware
  {
  public:
    coco_middleware(coco_core &cc);
    virtual ~coco_middleware() = default;

    /**
     * @brief Connects the middleware.
     *
     */
    virtual void connect() = 0;
    /**
     * @brief Disconnects the middleware.
     *
     */
    virtual void disconnect() = 0;

    /**
     * @brief Subscribes to the given topic.
     *
     * @param topic the topic to subscribe to.
     * @param qos the quality of service.
     */
    virtual void subscribe(const std::string &topic, bool local = true, int qos = 0) = 0;
    /**
     * @brief Unsubscribes from the given topic.
     *
     * @param topic the topic on which to publish.
     * @param msg the message to publish.
     * @param qos the quality of service.
     * @param retained whether the message is retained.
     */
    virtual void publish(const std::string &topic, const json::json &msg, bool local = true, int qos = 0, bool retained = false) = 0;

  protected:
    /**
     * @brief Called when a message is received.
     *
     * @param topic the topic of the message.
     * @param msg the message.
     */
    void message_arrived(const std::string &topic, json::json &msg);

  protected:
    coco_core &cc;
  };

  using coco_middleware_ptr = std::unique_ptr<coco_middleware>;
} // namespace coco
