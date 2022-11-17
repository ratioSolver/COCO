#include "sensor_network.h"
#include "logging.h"

namespace coco
{
    sensor_network::sensor_network(const std::string &root, const std::string &mqtt_uri, const std::string &mongodb_uri) : root(root)
    {
        std::cout << mqtt_uri << std::endl;
    }
    sensor_network::~sensor_network() {}
} // namespace coco
