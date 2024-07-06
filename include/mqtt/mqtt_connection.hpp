#pragma once

#include "json.hpp"
#include "mqtt/async_client.h"

#define MQTT_URI(host, port) "mqtt://" host ":" port

namespace coco
{
  class coco_core;
  class parameter;
  class type;
  class item;

  class mqtt_connection
  {
  public:
    mqtt_connection(coco_core &core, const std::string &mqtt_uri = MQTT_URI(MQTT_HOST, MQTT_PORT), const std::string &client_id = COCO_NAME);

    void new_parameter(const parameter &par);
    void deleted_parameter(const std::string &par_id);

    void new_type(const type &tp);
    void deleted_type(const std::string &tp_id);

    void new_item(const item &itm);
    void deleted_item(const std::string &itm_id);

    void new_data(const item &itm, const std::chrono::system_clock::time_point &timestamp, const json::json &data);

  private:
    void on_connect(const std::string &cause);
    void on_connection_lost(const std::string &cause);
    void on_message(mqtt::const_message_ptr msg);

  private:
    coco_core &core;
    mqtt::async_client client;
    mqtt::connect_options conn_opts;
  };
} // namespace coco
