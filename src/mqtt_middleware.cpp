#include "mqtt_middleware.h"
#include "coco.h"
#include "coco_db.h"
#include "logging.h"

namespace coco
{
    mqtt_middleware::mqtt_middleware(coco &cc, const std::string &mqtt_uri) : coco_middleware(cc), mqtt_client(mqtt_uri, cc.get_database().get_root() + "-client")
    {
        options.set_clean_session(true);
        options.set_keep_alive_interval(20);

        mqtt_client.set_callback(*this);
    }
    mqtt_middleware::~mqtt_middleware() {}

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

    void mqtt_middleware::publish(const std::string &topic, const json::json &msg, int qos, bool retained) { mqtt_client.publish(mqtt::make_message(topic, msg.dump(), qos, retained)); }

    void mqtt_middleware::connected(const std::string &cause) { LOG("MQTT client connected!"); }

    void mqtt_middleware::connection_lost(const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        mqtt_client.reconnect()->wait();
    }

    void mqtt_middleware::message_arrived(mqtt::const_message_ptr msg)
    {
        auto j_msg = json::load(msg->get_payload());
        coco_middleware::message_arrived(j_msg);
    }
} // namespace coco
