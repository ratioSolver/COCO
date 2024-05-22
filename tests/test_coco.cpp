#include "coco_core.hpp"
#include "mongo_db.hpp"

using namespace std::chrono_literals;

int main(int argc, char const *argv[])
{
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;

    db.drop(); // Warning!! We are deleting all the current data!!
    std::vector<coco::parameter_ptr> parameters;
    parameters.push_back(std::make_unique<coco::float_parameter>("battery", 0, 5));
    parameters.push_back(std::make_unique<coco::float_parameter>("temp", -100, 100));
    auto temp_type_id = db.create_item_type("temperature", "A type of item for measuring temperature", std::move(parameters));

    auto temp0_id = db.create_item("Temp0", db.get_item_type(temp_type_id), std::make_unique<coco::location>(37.5078, 15.083));

    coco::coco_core cc(db);
    cc.load_rules({"rules/rules.clp"});
    cc.start();

    std::this_thread::sleep_for(10s);
    return 0;
}
