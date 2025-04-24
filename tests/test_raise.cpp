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

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
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
#ifdef ENABLE_SSL
    srv.load_certificate("cert.pem", "key.pem");
#endif
    auto srv_ft = std::async(std::launch::async, [&srv]
                             { srv.server::start(); });
#endif

    std::string user_input;
    std::cin >> user_input;
    if (user_input == "d")
        db.drop();

#ifdef BUILD_SERVER
    srv.stop();
#endif

    return 0;
}
