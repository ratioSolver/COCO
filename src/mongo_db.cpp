#include "mongo_db.h"
#include "coco.h"

namespace coco
{
    mongo_db::mongo_db(const std::string &root, const std::string &mongodb_uri) : coco_db(root), conn{mongocxx::uri{mongodb_uri}}, db(conn[root]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["coco"]), sensor_data_collection(db["sensor_data"]) {}

    std::string mongo_db::create_sensor_type(const std::string &name, const std::string &description) {}
    void mongo_db::set_sensor_type_name(const std::string &id, const std::string &name) {}
    void mongo_db::set_sensor_type_description(const std::string &id, const std::string &description) {}
    void mongo_db::delete_sensor_type(const std::string &id) {}

    std::string mongo_db::create_sensor(const std::string &name, const sensor_type &type, std::unique_ptr<location> l) {}
    void mongo_db::set_sensor_name(const std::string &id, const std::string &name) {}
    void mongo_db::set_sensor_type(const std::string &id, const sensor_type &type) {}
    void mongo_db::set_sensor_location(const std::string &id, std::unique_ptr<location> l) {}
    void mongo_db::set_sensor_value(const std::string &id, std::unique_ptr<json::json> v) {}
    void mongo_db::delete_sensor(const std::string &id) {}
} // namespace coco
