#include "mqtt_middleware.h"
#include "coco_core.h"
#include "coco_db.h"
#include "logging.h"

namespace coco
{
    mqtt_middleware::mqtt_middleware(coco_core &cc, const std::string &mqtt_instance_uri, const std::string &mqtt_app_uri) : coco_middleware(cc), app_cb(*this), mqtt_app_client(mqtt_app_uri, cc.get_database().get_instance() + "-client"), instance_cb(*this), mqtt_instance_client(mqtt_instance_uri, cc.get_database().get_app() + "-" + cc.get_database().get_instance() + "-client")
    {
        options.set_clean_session(true);
        options.set_keep_alive_interval(20);

#ifdef MQTT_AUTH
        options.set_user_name(MQTT_USERNAME);
        options.set_password(MQTT_PASSWORD);
#endif

        mqtt_app_client.set_callback(app_cb);
        mqtt_instance_client.set_callback(instance_cb);
    }

    void mqtt_middleware::connect()
    {
        try
        {
            LOG("Connecting MQTT client to " + mqtt_app_client.get_server_uri() + "..");
            mqtt_app_client.connect(options)->wait();
            LOG("MQTT client connected!");

            LOG("Connecting MQTT client to " + mqtt_instance_client.get_server_uri() + "..");
            mqtt_instance_client.connect(options)->wait();
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
            LOG("Disconnecting MQTT client from " + mqtt_app_client.get_server_uri() + "..");
            mqtt_app_client.disconnect()->wait();
            LOG("MQTT client disconnected!");

            LOG("Disconnecting MQTT client from " + mqtt_instance_client.get_server_uri() + "..");
            mqtt_instance_client.disconnect()->wait();
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
            mqtt_instance_client.subscribe(topic, qos);
        }
        else
        {
            LOG_DEBUG("Subscribing to '" + topic + "' topic..");
            mqtt_app_client.subscribe(topic, qos);
        }
    }
    void mqtt_middleware::publish(const std::string &topic, const json::json &msg, bool local, int qos, bool retained)
    {
        if (local)
        {
            LOG_DEBUG("Publishing to '" + topic + "' topic..");
            mqtt_instance_client.publish(mqtt::make_message(topic, msg.to_string(), qos, retained));
        }
        else
        {
            LOG_DEBUG("Publishing to '" + topic + "' topic..");
            mqtt_app_client.publish(mqtt::make_message(topic, msg.to_string(), qos, retained));
        }
    }

    mqtt_middleware::app_callback::app_callback(mqtt_middleware &mm) : mm(mm) {}

    void mqtt_middleware::app_callback::connected([[maybe_unused]] const std::string &cause) { LOG("MQTT client connected!"); }

    void mqtt_middleware::app_callback::connection_lost([[maybe_unused]] const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        mm.mqtt_app_client.reconnect()->wait();
        LOG("MQTT client reconnected!");
    }

    void mqtt_middleware::app_callback::message_arrived(mqtt::const_message_ptr msg)
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

    mqtt_middleware::instance_callback::instance_callback(mqtt_middleware &mm) : mm(mm) {}

    void mqtt_middleware::instance_callback::connected([[maybe_unused]] const std::string &cause) { LOG("MQTT client connected!"); }

    void mqtt_middleware::instance_callback::connection_lost([[maybe_unused]] const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        mm.mqtt_instance_client.reconnect()->wait();
        LOG("MQTT client reconnected!");
    }

    void mqtt_middleware::instance_callback::message_arrived(mqtt::const_message_ptr msg)
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
