# COCO

deduCtiOn and abduCtiOn (COCO) reasoner.

## How to use

Create your application class that inherits from `coco::coco_core` and pass the database to the base class constructor.

```cpp
#include "coco_core.h"

class my_app : public coco::coco_core
{
  public:
    my_app(coco::coco_db &db) : coco_core(db) {}
};
```

Create a listener class that inherits from `coco::coco_listener` and pass the application to the base class constructor.

```cpp
#include "coco_listener.h"

class my_listener : public coco::coco_listener
{
  public:
    my_listener(my_app& app) : coco_listener(app) {}
};
```

Create a database class that inherits from a `coco::coco_db` implementation.

```cpp
#include "mongo_db.h"

class my_db : public coco::mongo_db
{
  public:
    my_db(const std::string &root = COCO_ROOT, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) : mongo_db(root, mongodb_uri) {}
};
```