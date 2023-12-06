#include "mqtt_middleware.h"
#include "coco_core.h"
#include "coco_db.h"
#include "logging.h"

namespace coco
{
    mqtt_middleware::mqtt_middleware(coco_core &cc, const std::string &mqtt_uri) : coco_middleware(cc), cb(*this), mqtt_client(mqtt_uri, cc.get_database().get_name() + "-client")
    {
        options.set_clean_session(true);
        options.set_keep_alive_interval(20);

#ifdef MQTT_AUTH
        options.set_user_name(MQTT_USERNAME);
        options.set_password(MQTT_PASSWORD);
#endif

        mqtt_client.set_callback(cb);
    }

    void mqtt_middleware::connect()
    {
        try
        {
            LOG("Connecting MQTT client to " + mqtt_client.get_server_uri() + "..");
            mqtt_client.connect(options)->wait();
            LOG("MQTT client connected!");
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
            LOG("MQTT client disconnected!");
        }
        catch (const mqtt::exception &e)
        {
            LOG_ERR(e.what());
        }
    }

    void mqtt_middleware::subscribe(const std::string &topic, bool local, int qos)
    {
        if (local)
        {
            LOG_DEBUG("Subscribing to '" + topic + "' topic..");
            mqtt_client.subscribe(topic, qos);
        }
        else
        {
            LOG_DEBUG("Subscribing to '" + topic + "' topic..");
            mqtt_client.subscribe(topic, qos);
        }
    }
    void mqtt_middleware::publish(const std::string &topic, const json::json &msg, bool local, int qos, bool retained)
    {
        if (local)
        {
            LOG_DEBUG("Publishing to '" + topic + "' topic..");
            mqtt_client.publish(mqtt::make_message(topic, msg.to_string(), qos, retained));
        }
        else
        {
            LOG_DEBUG("Publishing to '" + topic + "' topic..");
            mqtt_client.publish(mqtt::make_message(topic, msg.to_string(), qos, retained));
        }
    }

    mqtt_middleware::callback::callback(mqtt_middleware &mm) : mm(mm) {}

    void mqtt_middleware::callback::connected([[maybe_unused]] const std::string &cause) { LOG("MQTT client connected!"); }

    void mqtt_middleware::callback::connection_lost([[maybe_unused]] const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        mm.mqtt_client.reconnect();
    }

    void mqtt_middleware::callback::message_arrived(mqtt::const_message_ptr msg)
    {
        try
        {
            auto j_msg = json::load(msg->get_payload());
            mm.coco_middleware::message_arrived(msg->get_topic(), j_msg);
        }
        catch (const std::invalid_argument &e)
        {
            LOG_ERR("Invalid JSON message received from MQTT broker: " + std::string(e.what()));
        }
    }
} // namespace coco
