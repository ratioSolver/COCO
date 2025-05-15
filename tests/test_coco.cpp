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
    coco::coco_llm &llm = cc.add_module<coco::coco_llm>(cc);
    llm.create_intent("greet", "The user greets the system");
    llm.create_intent("bye", "The user says goodbye to the system");
    llm.create_entity(coco::string_type, "name", "The name of the user");
    llm.create_entity(coco::integer_type, "age", "The age of the user");
    llm.create_entity(coco::string_type, "robot_response", "A message in italian indicating a generated response or a comment to the user's input");
    llm.create_slot(coco::string_type, "name", "The name of the user");
    llm.create_slot(coco::integer_type, "age", "The age of the user");
    cc.create_reactive_rule("set_name", "(defrule set_name ?f <- (entity (item_id ?id) (name name) (value ?value)) => (set_slots ?id (create$ name) (create$ ?value)) (retract ?f))");
    cc.create_reactive_rule("set_age", "(defrule set_age ?f <- (entity (item_id ?id) (name age) (value ?value)) => (set_slots ?id (create$ age) (create$ ?value)) (retract ?f))");
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.server::start(); });
#ifdef ENABLE_SSL
    srv.load_certificate("cert.pem", "key.pem");
#endif
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

    auto &tp = cc.create_type("ParentType", {}, json::json(), json::json{{"online", {"type", "bool"}}});
    auto &itm_0 = cc.create_item(tp);
    cc.set_value(itm_0, json::json{{"online", true}});
    auto &itm_1 = cc.create_item(tp);
    cc.set_value(itm_1, json::json{{"online", false}});

    auto &ch_tp = cc.create_type("ChildType", {tp}, json::json(), json::json{{"count", {{"type", "int"}, {"min", 0}}}, {"temp", {{"type", "float"}, {"max", 50}}}});
    auto &ch_itm = cc.create_item(ch_tp);
    cc.set_value(ch_itm, json::json{{"online", true}, {"count", 1}, {"temp", 22.5}});

    auto &s_tp = cc.create_type("SourceType", {}, json::json(), json::json{{"parent", {{"type", "item"}, {"domain", "ParentType"}}}});
    auto &s_itm = cc.create_item(s_tp);
    cc.set_value(s_itm, json::json{{"parent", ch_itm.get_id().c_str()}});

    std::string user_input;
    do
    {
        std::cout << "Enter a command (d to drop the database, q to quit): ";
        std::getline(std::cin, user_input);
        if (user_input == "d")
            db.drop();
#ifdef BUILD_LLM
        else
            llm.understand(itm_0, user_input);
#endif
    } while (user_input != "d" && user_input != "q");

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
