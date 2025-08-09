#pragma once

#include "coco_module.hpp"
#include "coco.hpp"
#include "mqtt/async_client.h"

#define MQTT_URI(host, port) "mqtt://" host ":" port

namespace coco
{
  constexpr const int QOS = 1; // Quality of Service level for MQTT messages

  class coco_mqtt final : public coco_module, private listener
  {
  public:
    coco_mqtt(coco &cc, std::string_view mqtt_uri = MQTT_URI(MQTT_HOST, MQTT_PORT), std::string_view client_id = COCO_NAME) noexcept;

  private:
    void on_connect(const std::string &cause);
    void on_connection_lost(const std::string &cause);
    void on_message(mqtt::const_message_ptr msg);

    void created_type(const type &tp) override;
    void created_item(const item &itm) override;
    void updated_item(const item &itm) override;
    void new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp) override;

  private:
    mqtt::async_client client;       // MQTT client instance
    mqtt::connect_options conn_opts; // Connection options for MQTT
  };
} // namespace coco
