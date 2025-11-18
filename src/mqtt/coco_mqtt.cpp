#include "coco_mqtt.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#include "logging.hpp"

namespace coco
{
    coco_mqtt::coco_mqtt(coco &cc, std::string_view mqtt_uri, std::string_view client_id) noexcept : coco_module(cc), listener(cc), client(mqtt_uri.data(), client_id.data())
    {
        conn_opts.set_keep_alive_interval(20);
        conn_opts.set_clean_session(true);
        conn_opts.set_mqtt_version(MQTTVERSION_5);
#ifdef MQTT_AUTH
        auto user = std::getenv("MQTT_USER");
        if (user)
            conn_opts.set_user_name(user);
        auto pass = std::getenv("MQTT_PASSWORD");
        if (pass)
            conn_opts.set_password(pass);
#endif

        client.set_connected_handler(std::bind(&coco_mqtt::on_connect, this, std::placeholders::_1));
        client.set_connection_lost_handler(std::bind(&coco_mqtt::on_connection_lost, this, std::placeholders::_1));
        client.set_message_callback(std::bind(&coco_mqtt::on_message, this, std::placeholders::_1));

        LOG_DEBUG("Connecting to " << mqtt_uri);
        try
        {
            client.connect(conn_opts);
        }
        catch (const mqtt::exception &e)
        {
            LOG_ERR("Unable to connect to MQTT broker: " << e.what());
        }
    }

    void coco_mqtt::on_message(mqtt::const_message_ptr msg)
    {
        LOG_DEBUG("Received message on topic: " << msg->get_topic() << " with payload: " << msg->to_string());

        // Handle incoming messages based on the topic
        if (msg->get_topic().find(COCO_NAME "/data/") == 0)
            get_coco().set_value(get_coco().get_item(msg->get_topic().substr(strlen(COCO_NAME "/data/"))), json::load(msg->to_string())); // Set value for the item based on the topic
    }

    void coco_mqtt::on_connect(const std::string &cause)
    {
        LOG_DEBUG("Connected to MQTT broker: " << cause);

        for (auto &tp : get_coco().get_types())
            client.publish(COCO_NAME "/types/" + tp.get().get_name(), tp.get().to_json().dump(), QOS, true); // Publish each type

        mqtt::subscribe_options opts;
        opts.set_no_local(true); // Prevent receiving messages from self
        for (auto &itm : get_coco().get_items())
        {
            client.publish(COCO_NAME "/items/" + itm.get().get_id(), itm.get().to_json().dump(), QOS, true); // Publish each item
            client.subscribe(COCO_NAME "/data/" + itm.get().get_id(), QOS, opts);                            // Subscribe to data updates for each item
        }
    }
    void coco_mqtt::on_connection_lost(const std::string &cause)
    {
        LOG_ERR("Connection lost: " << cause);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        LOG_DEBUG("Reconnecting to MQTT broker...");
        try
        {
            client.connect(conn_opts);
            LOG_DEBUG("Reconnected to MQTT broker");
        }
        catch (const mqtt::exception &e)
        {
            LOG_ERR("Failed to reconnect: " << e.what());
        }
    }

    void coco_mqtt::created_type(const type &tp)
    {
        client.publish(COCO_NAME "/types/" + tp.get_name(), tp.to_json().dump(), QOS, true); // Publish the new type
    }
    void coco_mqtt::created_item(const item &itm)
    {
        client.publish(COCO_NAME "/items/" + itm.get_id(), itm.to_json().dump(), QOS, true); // Publish the new item
        mqtt::subscribe_options opts;
        opts.set_no_local(true);                                        // Prevent receiving messages from self
        client.subscribe(COCO_NAME "/data/" + itm.get_id(), QOS, opts); // Subscribe to data updates for the new item
    }
    void coco_mqtt::updated_item(const item &itm)
    {
        client.publish(COCO_NAME "/items/" + itm.get_id(), itm.to_json().dump(), QOS, true); // Publish the updated item
    }
    void coco_mqtt::new_data(const item &itm, const json::json &data, const std::chrono::system_clock::time_point &timestamp)
    {
        json::json j_data = {{"data", data}, {"timestamp", std::chrono::duration_cast<std::chrono::milliseconds>(timestamp.time_since_epoch()).count()}};
        client.publish(COCO_NAME "/data/" + itm.get_id(), j_data.dump(), QOS, false); // Publish new data for the item
    }
} // namespace coco
