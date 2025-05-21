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
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#include "auth_db.hpp"
#else
#include "coco_noauth.hpp"
#endif

int main()
{
#ifdef BUILD_MONGODB
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;
#else
    coco::coco_db db;
#endif
    coco::coco cc(db);

#ifdef BUILD_AUTH
    coco::coco_auth &auth = cc.add_module<coco::coco_auth>(cc);
#endif

#ifdef BUILD_LLM
    coco::coco_llm &llm = cc.add_module<coco::coco_llm>(cc);
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.server::start(); });
#ifdef ENABLE_SSL
    srv.load_certificate("tests/cert.pem", "tests/key.pem");
#endif
#ifdef BUILD_AUTH
    srv.add_module<coco::server_auth>(srv);
#else
    srv.add_module<coco::server_noauth>(srv);
#endif
#ifdef BUILD_LLM
    srv.add_module<coco::llm_server>(srv, llm);
#endif
    std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

#ifdef BUILD_LLM
    llm.create_intent("affirmative", "The user agrees with the system");
    llm.create_intent("negative", "The user disagrees with the system");
    llm.create_intent("greet", "The user greets the system");
    llm.create_intent("farewell", "The user says goodbye to the system");
    llm.create_entity(coco::string_type, "user_name", "The name of the user");
    llm.create_entity(coco::integer_type, "user_age", "The age of the user");
    llm.create_entity(coco::string_type, "robot_response", "A message in italian answering, commenting or requesting for clarification to the user's input, in the same tone used by the user. Try to be creative and funny. Always set this entity.");
    llm.create_entity(coco::string_type, "robot_face", "The face of the robot. Set this entity to a string among the following: 'happy', 'sad', 'angry', 'surprised', 'neutral'. This entity is used to set the face of the robot. Always set this entity.");
    llm.create_entity(coco::boolean_type, "robot_ask", "Set this entity to true if the robot is asking a question. Set this entity to false if the robot is not asking a question. Always set this entity.");
    llm.create_slot(coco::string_type, "user_name", "The name of the user");
    llm.create_slot(coco::integer_type, "user_age", "The age of the user");
    cc.create_reactive_rule("set_name", "(defrule set_name ?f <- (entity (item_id ?id) (name user_name) (value ?value)) => (set_slots ?id (create$ user_name) (create$ ?value)) (retract ?f))");
    cc.create_reactive_rule("set_age", "(defrule set_age ?f <- (entity (item_id ?id) (name user_age) (value ?value)) => (set_slots ?id (create$ user_age) (create$ ?value)) (retract ?f))");
#endif

    auto &user_tp = cc.create_type("User", {}, json::json{{"name", {"type", "string"}}, {"age", {"type", "int"}}}, {});
    [[maybe_unused]] auto &user_itm = cc.create_item(user_tp);

    auto &robot_tp = cc.create_type("Robot", {}, {}, json::json{{"saying", {"type", "string"}}, {"robot_face", {{"type", "symbol"}, {"values", {"happy", "sad", "angry", "surprised", "neutral"}}}}, {"user", {{"type", "item"}, {"domain", "User"}}}, {"listening", {"type", "bool"}}});
    [[maybe_unused]] auto &robot_itm = cc.create_item(robot_tp);

#ifdef BUILD_SERVER
    std::this_thread::sleep_for(std::chrono::seconds(100));
    db.drop();
    srv.stop();
#endif
}
