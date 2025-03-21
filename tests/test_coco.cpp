#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#ifdef BUILD_TRANSFORMER
#include "coco_transformer.hpp"
#endif
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#endif
#if defined(BUILD_SERVER) || defined(NO_INTERACTIVE_MODE)
#include <thread>
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
#ifdef BUILD_MONGODB
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;
#else
    coco::coco_db db;
#endif
    coco::coco cc(db);

#ifdef BUILD_TRANSFORMER
    coco::transformer t(cc);
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.server::start(); });
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

#ifndef NO_INTERACTIVE_MODE
    std::this_thread::sleep_for(std::chrono::seconds(1));
    db.drop();
#else
    bool skip_user_input = false;
    if (argc > 1 && std::string(argv[1]) == "--skip-input")
        skip_user_input = true;

    if (!skip_user_input)
    {
        std::string user_input;
        std::cin >> user_input;
        if (user_input == "d")
            db.drop();
    }
#endif

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
