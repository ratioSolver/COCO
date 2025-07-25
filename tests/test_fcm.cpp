#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#ifdef BUILD_MONGODB
#include "mongo_db.hpp"
#include <mongocxx/instance.hpp>
#else
#include "coco_db.hpp"
#endif
#ifdef BUILD_FCM
#include "coco_fcm.hpp"
#endif
#ifdef BUILD_SERVER
#include "coco_server.hpp"
#ifdef BUILD_AUTH
#include "coco_auth.hpp"
#else
#include "coco_noauth.hpp"
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

#ifdef BUILD_FCM
    [[maybe_unused]] auto &fcm = cc.add_module<coco::coco_fcm>(cc);
#endif

#ifdef BUILD_AUTH
    cc.add_module<coco::coco_auth>(cc);
#endif

#ifdef BUILD_SERVER
    coco::coco_server srv(cc);
#ifdef ENABLE_SSL
    srv.load_certificate("cert.pem", "key.pem");
#endif
#ifdef BUILD_AUTH
    srv.add_module<coco::server_auth>(srv);
    srv.add_middleware<coco::auth_middleware>(srv);
#else
    srv.add_module<coco::server_noauth>(srv);
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
