#include "coco_core.h"
#include "mongo_db.h"
#include "mqtt_middleware.h"

using namespace std::chrono_literals;

int main(int argc, char const *argv[])
{
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;

    db.drop(); // Warning!! We are deleting all the current data!!
    auto temp_type_id = db.create_sensor_type("temperature", "A type of sensor for measuring temperature", {{"battery", coco::parameter_type::Float}, {"temp", coco::parameter_type::Float}});
    auto temp0_id = db.create_sensor("Temp0", db.get_sensor_type(temp_type_id), std::make_unique<coco::location>(37.5078, 15.083));

    coco::coco_core cc(db);
    cc.add_middleware(std::make_unique<coco::mqtt_middleware>(cc));
    cc.load_rules({"rules/rules.clp"});
    cc.connect();

    std::this_thread::sleep_for(10s);

    cc.disconnect();
    return 0;
}
