#pragma once

#include "coco_middleware.h"
#include "mqtt/async_client.h"

#define MQTT_URI(host, port) host ":" port

namespace coco
{
  class mqtt_middleware : public coco_middleware
  {
  public:
    mqtt_middleware(coco_core &cc, const std::string &mqtt_instance_uri = MQTT_URI(MQTT_INSTANCE_HOST, MQTT_INSTANCE_PORT), const std::string &mqtt_app_uri = MQTT_URI(MQTT_APP_HOST, MQTT_APP_PORT));

    mqtt::connect_options &get_options() { return options; }

    void connect() override;
    void disconnect() override;

    void subscribe(const std::string &topic, bool local = true, int qos = 0) override;
    void publish(const std::string &topic, const json::json &msg, bool local = true, int qos = 0, bool retained = false) override;

    class app_callback : public mqtt::callback
    {
    public:
      app_callback(mqtt_middleware &mm);

    private:
      void connected(const std::string &cause) override;
      void connection_lost(const std::string &cause) override;
      void message_arrived(mqtt::const_message_ptr msg) override;

    private:
      mqtt_middleware &mm;
    };

    class instance_callback : public mqtt::callback
    {
    public:
      instance_callback(mqtt_middleware &mm);

    private:
      void connected(const std::string &cause) override;
      void connection_lost(const std::string &cause) override;
      void message_arrived(mqtt::const_message_ptr msg) override;

    private:
      mqtt_middleware &mm;
    };

  private:
    app_callback app_cb;
    mqtt::async_client mqtt_app_client;
    instance_callback instance_cb;
    mqtt::async_client mqtt_instance_client;
    mqtt::connect_options options;
  };
} // namespace coco
