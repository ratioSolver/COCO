#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#include <thread>
#endif

void test_basic_core()
{
#ifdef BUILD_MONGODB
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;
#else
    coco::coco_db db;
#endif
    coco::coco cc(db);

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.start(); });
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    auto &tp = cc.create_type("ParentType", {}, json::json(), json::json{{"online", {"type", "bool"}}});
    auto &itm = cc.create_item(tp);
    cc.set_value(itm, json::json{{"online", true}});

    auto &ch_tp = cc.create_type("ChildType", {tp}, json::json(), json::json{{"count", {{"type", "int"}, {"min", 0}}}, {"temp", {{"type", "float"}, {"max", 50}}}});
    auto &ch_itm = cc.create_item(ch_tp);
    cc.set_value(ch_itm, json::json{{"online", true}, {"count", 1}, {"temp", 22.5}});

    auto &s_tp = cc.create_type("SourceType", {}, json::json(), json::json{{"parent", {{"type", "item"}, {"domain", "ParentType"}}}});
    auto &s_itm = cc.create_item(s_tp);
    cc.set_value(s_itm, json::json{{"parent", ch_itm.get_id().c_str()}});

    db.drop();

    srv.stop();
}

int main()
{
    test_basic_core();

    return 0;
}
