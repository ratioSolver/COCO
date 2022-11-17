#include "sensor_network.h"
#include "logging.h"

namespace coco
{
    mqtt_callback::mqtt_callback(sensor_network &sn) : sn(sn) {}

    void mqtt_callback::connected([[maybe_unused]] const std::string &cause)
    {
        LOG("MQTT client connected!");
    }
    void mqtt_callback::connection_lost([[maybe_unused]] const std::string &cause)
    {
        LOG_WARN("MQTT connection lost! trying to reconnect..");
        sn.mqtt_client.reconnect()->wait();
    }
    void mqtt_callback::message_arrived(mqtt::const_message_ptr msg)
    {
    }

    sensor_network::sensor_network(const std::string &root, const std::string &mqtt_uri, const std::string &mongodb_uri) : root(root), mqtt_client(mqtt_uri, "client_id"), msg_callback(*this), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["sensor_network"]), sensor_data_collection(db["sensor_data"])
    {
        std::cout << mqtt_uri << std::endl;
    }
    sensor_network::~sensor_network() {}

    void sensor_network::update_sensor_network(json::json msg)
    {
    }
} // namespace coco
