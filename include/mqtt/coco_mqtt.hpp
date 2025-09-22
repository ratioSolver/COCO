#pragma once

#include "coco_module.hpp"
#include "coco.hpp"
#include "mqtt/async_client.h"

#ifdef MQTT_SECURE
#define MQTT_PROTOCOL "mqtts"
#else
#define MQTT_PROTOCOL "mqtt"
#endif

#ifdef MQTT_AUTH
#define MQTT_URI(user, pass, host, port) MQTT_PROTOCOL "://" user ":" pass "@" host ":" port
#define MQTT_DEFAULT_URI MQTT_URI(MQTT_USER, MQTT_PASSWORD, MQTT_HOST, MQTT_PORT)
#else
#define MQTT_URI(host, port) MQTT_PROTOCOL "://" host ":" port
#define MQTT_DEFAULT_URI MQTT_URI(MQTT_HOST, MQTT_PORT)
#endif

namespace coco
{
  constexpr const int QOS = 1; // Quality of Service level for MQTT messages

  class coco_mqtt : public coco_module, private listener
  {
  public:
    coco_mqtt(coco &cc, std::string_view mqtt_uri = MQTT_DEFAULT_URI, std::string_view client_id = COCO_NAME) noexcept;

    bool is_connected() const noexcept { return client.is_connected(); }

  protected:
    virtual void on_message(mqtt::const_message_ptr msg);
    virtual void on_connect(const std::string &cause);

  private:
    void on_connection_lost(const std::string &cause);

    void created_type(const type &tp) override;
    void created_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

  protected:
    mqtt::async_client client; // MQTT client instance

  private:
    mqtt::connect_options conn_opts; // Connection options for MQTT
  };
} // namespace coco
