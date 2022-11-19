#include "coco.h"
#include "mongo_db.h"

int main(int argc, char const *argv[])
{
    coco::mongo_db db;
    coco::coco cc(db);
    return 0;
}
