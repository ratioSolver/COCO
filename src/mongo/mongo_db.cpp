#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <cassert>
#include "mongo_db.hpp"

namespace coco
{
    mongo_db::mongo_db(const json::json &config, const std::string &mongodb_uri) : coco_db(config), conn(mongocxx::uri(mongodb_uri)), db(conn[static_cast<std::string>(config["name"])]), sensor_types_collection(db["sensor_types"]), sensors_collection(db["sensors"]), sensor_data_collection(db["sensor_data"]) {}
} // namespace coco
