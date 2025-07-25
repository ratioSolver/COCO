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
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#else
#include "coco_noauth.hpp"
#endif
#include <thread>
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
    cc.add_module<coco::coco_auth>(cc);
#endif

#ifdef BUILD_LLM
    [[maybe_unused]] coco::coco_llm &llm = cc.add_module<coco::coco_llm>(cc);
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.start(); });
#ifdef ENABLE_SSL
    srv.load_certificate("cert.pem", "key.pem");
#endif
#ifdef BUILD_AUTH
    srv.add_module<coco::server_auth>(srv);
    srv.add_middleware<coco::auth_middleware>(srv);
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
    llm.create_intent("presentation", "The user introduces himself/herself to the system");
    llm.create_intent("ask_something", "The user asks something to the system");
    llm.create_entity(coco::string_type, "user_name", "The name of the user. Don't set this entity if the user is not saying his/her name");
    llm.create_entity(coco::integer_type, "user_age", "The age of the user. Don't set this entity if the user is not saying his/her age");
    llm.create_entity(coco::string_type, "robot_response", "Always set this entity with a message in italian answering, commenting or requesting for clarification to the user's input, in the same tone used by the user. Try to be creative and funny.");
    llm.create_entity(coco::string_type, "robot_face", "Always set this entity with the face of the robot according to the robot's message. Set this entity to a string among: 'happy', 'sad', 'angry', 'surprised', 'neutral'. This entity is used to set the face of the robot.");
    llm.create_entity(coco::boolean_type, "robot_ask", "Always set this entity to true if the robot is asking a question. Set this entity to false if the robot is not asking a question.");
    llm.create_slot(coco::string_type, "user_name", "The name of the user");
    llm.create_slot(coco::integer_type, "user_age", "The age of the user");
#endif

    // Create the 'User' type
    auto &user_tp = cc.create_type("User", {}, json::json{{"name", {"type", "string"}}, {"age", {"type", "int"}}}, {});
    // Create a user
    auto &user_itm = cc.create_item(user_tp);

    // Create the 'Robot' type
    auto &robot_tp = cc.create_type("Robot", {}, {}, json::json{{"saying", {"type", "string"}}, {"understood", {"type", "string"}}, {"robot_face", {{"type", "symbol"}, {"values", {"happy", "sad", "angry", "surprised", "neutral"}}}}, {"user", {{"type", "item"}, {"domain", "User"}}}, {"listening", {"type", "bool"}}});
    // Create a robot
    auto &robot_itm = cc.create_item(robot_tp);

    // Set the user the robot is interacting with
    cc.set_value(robot_itm, {{"user", user_itm.get_id()}});

#ifdef BUILD_LLM
    // If someone talks to the robot, the robot has to understand the message
    cc.create_reactive_rule("understand", "(defrule understand (Robot_has_understood (item_id ?robot) (understood ?understood)) => (understand ?robot ?understood) (add_data ?robot (create$ understood) (create$ nil)))");
    // If the message contains the name of the user, set the name of the user
    cc.create_reactive_rule("set_name", "(defrule set_name ?f <- (entity (item_id ?robot) (name user_name) (value ?value)) (Robot_has_user (item_id ?robot) (user ?user)) => (set_slots ?robot (create$ user_name) (create$ ?value)) (set_props ?user (create$ name) (create$ ?value)) (retract ?f))");
    // If the message contains the age of the user, set the age of the user
    cc.create_reactive_rule("set_age", "(defrule set_age ?f <- (entity (item_id ?robot) (name user_age) (value ?value)) (Robot_has_user (item_id ?robot) (user ?user)) => (set_slots ?robot (create$ user_age) (create$ ?value)) (set_props ?user (create$ age) (create$ ?value)) (retract ?f))");
    // If the message contains the robot's response, set the robot's response
    cc.create_reactive_rule("set_robot_response", "(defrule set_robot_response ?f <- (entity (item_id ?robot) (name robot_response) (value ?value)) => (add_data ?robot (create$ saying) (create$ ?value)) (retract ?f))");
    // If the message contains the robot's face, set the robot's face
    cc.create_reactive_rule("set_robot_face", "(defrule set_robot_face ?f <- (entity (item_id ?robot) (name robot_face) (value ?value)) => (add_data ?robot (create$ robot_face) (create$ ?value)) (retract ?f))");
    // If the message contains a question, set the robot at listening
    cc.create_reactive_rule("set_robot_listening", "(defrule set_robot_listening ?f <- (entity (item_id ?robot) (name robot_ask) (value ?value)) => (add_data ?robot (create$ listening) (create$ ?value)) (retract ?f))");
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
            cc.set_value(robot_itm, json::json{{"understood", user_input}});
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
