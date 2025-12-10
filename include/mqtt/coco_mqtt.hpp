#pragma once

#include "coco_module.hpp"
#include "coco.hpp"
#include "mqtt/async_client.h"

#ifdef MQTT_SECURE
#define MQTT_PROTOCOL "mqtts"
#else
#define MQTT_PROTOCOL "mqtt"
#endif

namespace coco
{
  constexpr const int QOS = 1;                    // Quality of Service level for MQTT messages

  [[nodiscard]] inline std::string default_mqtt_uri() noexcept
  {
    std::string uri = MQTT_PROTOCOL "://";
    const char *host = std::getenv("MQTT_HOST");
    if (host)
      uri += host;
    else
      uri += "localhost";
    uri += ":";
    const char *port = std::getenv("MQTT_PORT");
    if (port)
      uri += port;
    else
      uri += "1883";
    return uri;
  }

  class coco_mqtt : public coco_module, private listener
  {
  public:
    coco_mqtt(coco &cc, std::string_view mqtt_uri = default_mqtt_uri(), std::string_view client_id = COCO_NAME) noexcept;

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
