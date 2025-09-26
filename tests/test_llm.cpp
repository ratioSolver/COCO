#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#ifdef BUILD_LLM
#include "coco_llm.hpp"
#endif
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#ifdef BUILD_LLM
#include "llm_server.hpp"
#endif
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

#ifdef BUILD_LLM
    [[maybe_unused]] coco::coco_llm &llm = cc.add_module<coco::coco_llm>(cc);
    if (llm.get_intents().empty())
    {
        llm.create_intent("greet", "The user greets the system");
        llm.create_intent("bye", "The user says goodbye to the system");
        llm.create_entity(coco::string_type, "user_name", "The name of the user");
        llm.create_entity(coco::integer_type, "user_age", "The age of the user");
        llm.create_entity(coco::string_type, "robot_response", "A message in italian answering, commenting or requesting for clarification to the user's input, in the same tone used by the user. Try to be creative and funny.");
        llm.create_slot(coco::string_type, "user_name", "The name of the user");
        llm.create_slot(coco::integer_type, "user_age", "The age of the user");
        cc.create_reactive_rule("set_name", "(defrule set_name ?f <- (entity (item_id ?id) (name user_name) (value ?value)) => (set_slots ?id (create$ user_name) (create$ ?value)) (retract ?f))");
        cc.create_reactive_rule("set_age", "(defrule set_age ?f <- (entity (item_id ?id) (name user_age) (value ?value)) => (set_slots ?id (create$ user_age) (create$ ?value)) (retract ?f))");
    }
    else
        cc.load_rules();
#endif
    coco::item *itm_0;
    try
    {
        for (auto &r : cc.get_type("ParentType").get_instances())
        {
            itm_0 = &r.get();
            break;
        }
        cc.load_rules();
    }
    catch (const std::exception &e)
    {
        auto &tp = cc.create_type("ParentType", {}, json::json(), json::json{{"online", {"type", "bool"}}});
        itm_0 = &cc.create_item(tp);
        cc.set_value(*itm_0, json::json{{"online", true}});
        auto &itm_1 = cc.create_item(tp);
        cc.set_value(itm_1, json::json{{"online", false}});

        auto &ch_tp = cc.create_type("ChildType", {tp}, json::json(), json::json{{"count", {{"type", "int"}, {"min", 0}}}, {"temp", {{"type", "float"}, {"max", 50}}}});
        auto &ch_itm = cc.create_item(ch_tp);
        cc.set_value(ch_itm, json::json{{"online", true}, {"count", 1}, {"temp", 22.5}});

        auto &s_tp = cc.create_type("SourceType", {}, json::json(), json::json{{"parent", {{"type", "item"}, {"domain", "ParentType"}}}});
        auto &s_itm = cc.create_item(s_tp);
        cc.set_value(s_itm, json::json{{"parent", itm_1.get_id()}});
    }

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.start(); });
#ifdef BUILD_SECURE
    srv.load_certificate("cert.pem", "key.pem");
#endif
#ifdef BUILD_LLM
    srv.add_module<coco::llm_server>(srv, llm);
#endif
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

#ifdef INTERACTIVE_TEST
    std::string user_input;
    do
    {
        std::cout << "Enter a command (d to drop the database, q to quit): ";
        std::getline(std::cin, user_input);
        if (user_input == "d")
            db.drop();
#ifdef BUILD_LLM
        else
            llm.understand(*itm_0, user_input);
#endif
    } while (user_input != "d" && user_input != "q");
#else
    db.drop();
#endif

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
