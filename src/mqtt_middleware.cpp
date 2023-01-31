#include "mqtt_middleware.h"
#include "coco_core.h"
#include "coco_db.h"
#include "logging.h"

namespace coco
{
    mqtt_middleware::mqtt_middleware(coco_core &cc, const std::string &mqtt_uri) : coco_middleware(cc), mqtt_client(mqtt_uri, cc.get_database().get_root() + "-client")
    {
        options.set_clean_session(true);
        options.set_keep_alive_interval(20);

        mqtt_client.set_callback(*this);
    }

    void mqtt_middleware::connect()
    {
        LOG("Connecting MQTT client to " + mqtt_client.get_server_uri() + "..");
        try
        {
            mqtt_client.connect(options)->wait();
        }
        catch (const mqtt::exception &e)
        {
            LOG_ERR(e.what());
        }
    }
    void mqtt_middleware::disconnect()
    {
        try
        {
            LOG("Disconnecting MQTT client from " + mqtt_client.get_server_uri() + "..");
            mqtt_client.disconnect()->wait();
        }
        catch (const mqtt::exception &e)
        {
            LOG_ERR(e.what());
        }
    }

    void mqtt_middleware::subscribe(const std::string &topic, int qos)
    {
        LOG_DEBUG("Subscribing to '" + topic + "' topic..");
        mqtt_client.subscribe(topic, qos);
    }
    void mqtt_middleware::publish(const std::string &topic, const json::json &msg, int qos, bool retained) { mqtt_client.publish(mqtt::make_message(topic, msg.dump(), qos, retained)); }

    void mqtt_middleware::connected([[maybe_unused]] const std::string &cause) { LOG("MQTT client connected!"); }

    void mqtt_middleware::connection_lost([[maybe_unused]] const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        mqtt_client.reconnect()->wait();
    }

    void mqtt_middleware::message_arrived(mqtt::const_message_ptr msg)
    {
        try
        {
            auto j_msg = json::load(msg->get_payload());
            coco_middleware::message_arrived(msg->get_topic(), j_msg);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid JSON message received from MQTT broker: " + std::string(e.what()));
        }
    }
} // namespace coco
