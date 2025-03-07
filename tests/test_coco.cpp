#include "coco.hpp"
#ifdef ENABLE_MONGO_DB
#include "mongo_db.hpp"
#endif

int main()
{
#ifdef ENABLE_MONGO_DB
    mongocxx::instance inst{}; // This should be done only once.
    coco::mongo_db db;
#endif

    return 0;
}
