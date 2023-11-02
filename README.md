# COCO

deduCtiOn and abduCtiOn (COCO) reasoner.

## How to use

### The Database

Create a database class that inherits from a `coco::coco_db` implementation. The `coco::mongo_db` implementation is provided as an example.

```cpp
#include "mongo_db.h"

class my_db : public coco::mongo_db
{
  public:
    my_db(const std::string &root = COCO_NAME, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) : mongo_db(root, mongodb_uri) {}
};
```

### The Application

Create your application class that inherits from `coco::coco_core` and pass the database to the base class constructor.

```cpp
#include "coco_core.h"

class my_app : public coco::coco_core
{
  public:
    my_app(my_db &db) : coco_core(db) {}
};
```

The application can be used to extend the COCO reasoner with your own functionality, defining your own User Defined Functions (UDFs) and User Defined Types (UDTs).

### The Listener

Create a listener class that inherits from `coco::coco_listener` and pass the application to the base class constructor.

```cpp
#include "coco_listener.h"

class my_listener : public coco::coco_listener
{
  public:
    my_listener(my_app& app) : coco_listener(app) {}
};
```