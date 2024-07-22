#include "mqtt_connection.hpp"
#include "coco_core.hpp"
#include "logging.hpp"

namespace coco
{
    mqtt_connection::mqtt_connection(coco_core &core, const std::string &mqtt_uri, const std::string &client_id) : cc(core), client(mqtt_uri, client_id)
    {
        conn_opts.set_keep_alive_interval(20);
        conn_opts.set_clean_session(true);

        client.set_connected_handler(std::bind(&mqtt_connection::on_connect, this, std::placeholders::_1));
        client.set_connection_lost_handler(std::bind(&mqtt_connection::on_connection_lost, this, std::placeholders::_1));
        client.set_message_callback(std::bind(&mqtt_connection::on_message, this, std::placeholders::_1));

        LOG_INFO("Connecting to " << mqtt_uri);
        client.connect(conn_opts);
    }

    void mqtt_connection::new_type(const type &tp) { client.publish(COCO_NAME "/notifications/new_type", to_json(tp).dump(), 1, false); }
    void mqtt_connection::deleted_type(const std::string &tp_id) { client.publish(COCO_NAME "/notifications/deleted_type", tp_id, 1, false); }

    void mqtt_connection::new_item(const item &itm) { client.publish(COCO_NAME "/notifications/new_item", to_json(itm).dump(), 1, false); }
    void mqtt_connection::deleted_item(const std::string &itm_id)
    {
        client.unsubscribe(COCO_NAME "/commands/items/" + itm_id);
        client.publish(COCO_NAME "/notifications/deleted_item", itm_id, 1, false);
    }

    void mqtt_connection::new_data(const item &itm, const std::chrono::system_clock::time_point &, const json::json &data)
    {
        LOG_TRACE("Publishing data for " << itm.get_id());
        client.publish(COCO_NAME "/notifications/items/" + itm.get_id(), data.dump(), 1, true);
    }

    void mqtt_connection::on_connect(const std::string &cause)
    {
        if (!cause.empty())
        {
            LOG_WARN("Connection failed: " << cause);
            std::this_thread::sleep_for(std::chrono::milliseconds(2500));
            LOG_DEBUG("Reconnecting...");
            client.connect(conn_opts);
            return;
        }

        LOG_INFO("Connected to MQTT broker");
        for (const auto &itm : cc.get_items())
            if (!itm.get().get_type().get_dynamic_properties().empty())
            {
                LOG_DEBUG("Subscribing to " << itm.get().get_id());
                client.subscribe(COCO_NAME "/commands/items/" + itm.get().get_id(), 1);
            }

        client.subscribe(COCO_NAME "/commands/create_property", 1);
        client.subscribe(COCO_NAME "/commands/delete_property", 1);
        client.subscribe(COCO_NAME "/commands/create_type", 1);
        client.subscribe(COCO_NAME "/commands/delete_type", 1);
        client.subscribe(COCO_NAME "/commands/create_item", 1);
        client.subscribe(COCO_NAME "/commands/delete_item", 1);
    }

    void mqtt_connection::on_connection_lost(const std::string &cause)
    {
        LOG_WARN("Connection lost: " << cause);
        std::this_thread::sleep_for(std::chrono::milliseconds(2500));
        LOG_DEBUG("Reconnecting...");
        client.connect(conn_opts);
    }

    void mqtt_connection::on_message(mqtt::const_message_ptr msg)
    {
        LOG_TRACE("Message arrived: " << msg->get_topic() << " " << msg->to_string());
        std::string topic = msg->get_topic();
        if (topic.find(COCO_NAME "/commands/items/") == 0)
        { // data message
            std::string itm_id = topic.substr(strlen(COCO_NAME "/commands/items/"));
            itm_id = itm_id.substr(0, itm_id.find("/data"));
            cc.add_data(cc.get_item(itm_id), json::load(msg->to_string()));
        }
        else if (topic == COCO_NAME "/commands/create_type")
        { // create type
            json::json j = json::load(msg->to_string());
            std::vector<std::reference_wrapper<const type>> parents;
            if (j.contains("parents"))
                for (const auto &p : j["parents"].as_array())
                    parents.push_back(cc.get_type(p));
            std::vector<std::unique_ptr<property>> static_properties;
            if (j.contains("properties"))
                for (const auto &p : j["properties"].as_array())
                    static_properties.push_back(make_property(cc, p));
            std::vector<std::unique_ptr<property>> dynamic_properties;
            if (j.contains("dynamic_properties"))
                for (const auto &p : j["dynamic_properties"].as_array())
                    dynamic_properties.push_back(make_property(cc, p));
            cc.create_type(j["name"], j["description"], std::move(parents), std::move(static_properties), std::move(dynamic_properties));
        }
        else if (topic == COCO_NAME "/commands/delete_type") // delete type
            cc.delete_type(msg->to_string());
        else if (topic == COCO_NAME "/commands/create_item")
        { // create item
            json::json j = json::load(msg->to_string());
            cc.create_item(cc.get_type(j["type"]), j["name"], j["properties"]);
        }
        else if (topic == COCO_NAME "/commands/delete_item") // delete item
            cc.delete_item(msg->to_string());
    }
} // namespace coco
