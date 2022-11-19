#pragma once

#include "coco_middleware.h"
#include "mqtt/async_client.h"

#define MQTT_URI(host, port) host ":" port

namespace coco
{
  class mqtt_middleware : public coco_middleware, public mqtt::callback
  {
  public:
    mqtt_middleware(coco &cc, const std::string &mqtt_uri = MQTT_URI(MQTT_HOST, MQTT_PORT));

    mqtt::connect_options &get_options() { return options; }

    void connect() override;
    void disconnect() override;

    void publish(const std::string &topic, const json::json &msg, int qos = 0, bool retained = false) override;

  private:
    void connected(const std::string &cause) override;
    void connection_lost(const std::string &cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

  private:
    mqtt::async_client mqtt_client;
    mqtt::connect_options options;
  };
} // namespace coco
