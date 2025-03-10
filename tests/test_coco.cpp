#include "coco.hpp"
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

    return 0;
}
