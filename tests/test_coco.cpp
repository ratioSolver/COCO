#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#endif

int main()
{
#ifdef BUILD_MONGODB
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;
#endif
    coco::coco cc(db);

    auto &tp = cc.create_type("ParentType", json::json(), json::json{{"online", {"type", "bool"}}});
    auto &itm = tp.new_instance("0");
    itm.set_value(json::json{{"online", true}});

    auto &ch_tp = cc.create_type("ChildType", json::json(), json::json{{"count", {{"type", "int"}, {"min", 0}}}});
    ch_tp.set_parents({tp});
    auto &ch_itm = ch_tp.new_instance("1");
    ch_itm.set_value(json::json{{"online", true}, {"count", 1}});

    return 0;
}
