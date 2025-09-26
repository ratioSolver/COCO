#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#ifdef BUILD_DELIBERATIVE
#include "coco_deliberative.hpp"
#endif
#ifdef BUILD_MQTT
#include "coco_mqtt.hpp"
#endif
#ifdef BUILD_LLM
#include "coco_llm.hpp"
#endif
#ifdef BUILD_FCM
#include "coco_fcm.hpp"
#endif
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#ifdef ENABLE_CORS
#include "cors.hpp"
#endif
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#else
#include "coco_noauth.hpp"
#endif
#ifdef BUILD_LLM
#include "llm_server.hpp"
#endif
#ifdef BUILD_FCM
#include "fcm_server.hpp"
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

#ifdef BUILD_DELIBERATIVE
    cc.add_module<coco::coco_deliberative>(cc);
#endif
#ifdef BUILD_MQTT
    cc.add_module<coco::coco_mqtt>(cc);
#endif
#ifdef BUILD_LLM
    [[maybe_unused]] coco::coco_llm &llm = cc.add_module<coco::coco_llm>(cc);
#endif
#ifdef BUILD_FCM
    [[maybe_unused]] auto &fcm = cc.add_module<coco::coco_fcm>(cc);
#endif
    cc.load_rules();

#ifdef BUILD_AUTH
    cc.add_module<coco::coco_auth>(cc);
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
#ifdef ENABLE_CORS
    srv.add_middleware<network::cors>(srv);
#endif
#ifdef BUILD_SECURE
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
#ifdef BUILD_FCM
    srv.add_module<coco::fcm_server>(srv, fcm);
#endif
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.start(); });
#endif

#ifdef INTERACTIVE_TEST
    std::string user_input;
    std::cin >> user_input;
    if (user_input == "d")
        db.drop();
#else
    db.drop();
#endif

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
