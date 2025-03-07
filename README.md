# COCO

deduCtiOn and abduCtiOn (COCO) reasoner.

## How to use

Use the provided `new_coco_project.py` script to create a new project.

```bash
python3 scripts/new_coco_project.py <project_name>
```

### The Database

Implement a database class that inherits from a `coco::coco_db` implementation. The `coco::mongo_db` implementation is provided as an example.

```cpp
#include "mongo_db.h"

class my_db : public coco::mongo_db
{
  public:
    my_db(const std::string &root = COCO_NAME, const std::string &mongodb_uri = MONGODB_URI(MONGODB_HOST, MONGODB_PORT)) : mongo_db(root, mongodb_uri) {}
};
```

### The Application

Implement your application class that inherits from `coco::coco_core` and pass the database to the base class constructor.

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

Implement a listener class that inherits from `coco::coco_listener` and pass the application to the base class constructor.

```cpp
#include "coco_listener.h"

class my_listener : public coco::coco_listener
{
  public:
    my_listener(my_app& app) : coco_listener(app) {}
};
```

## Installation of COCO on a local machine

### CLIPS

COCO relies on [CLIPS](https://www.clipsrules.net) for reacting to the dynamic changes which happen into the urban environment.
 - Download [CLIPS v6.4.2](https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.2/clips_core_source_642.zip/download) and unzip the zip file into the `clips_core_source_642` folder.
 - Reach the `clips_core_source_642/core` folder and compile CLIPS through `make release_cpp`.
 - Copy all the header files into the `/usr/local/include/clips` folder through `sudo cp *.h /usr/local/include/clips/`.
 - Copy the library into the `/usr/local/lib` folder through `sudo cp libclips.a /usr/local/lib/`.

### MongoDB

COCO relies on [MongoDB](https://www.mongodb.com) for storing the data which is collected from the sensors and the participatory data. It is required, in particular, to install the [cxx drivers](https://www.mongodb.com/docs/drivers/cxx/) for connecting COCO to a MongoDB database.

**Installig OpenSSL**

```shell
sudo apt-get install libssl-dev
```

**Installing MongoDB cxx drivers**

Download and configure the mongoc driver.

```shell
git clone https://github.com/mongodb/mongo-c-driver.git
cd mongo-c-driver
python build/calc_release_version.py > VERSION_CURRENT
mkdir cmake-build
cd cmake-build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
```

Compile and install it.

```shell
cmake --build .
sudo cmake --build . --target install
```

Download the mongocxx driver.

```shell
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.9.0/mongo-cxx-driver-r3.9.0.tar.gz
tar -xzf mongo-cxx-driver-r3.9.0.tar.gz
cd mongo-cxx-driver-r3.9.0/build
```

Configure the driver.

```shell
cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF
```

Build and install il.

```shell
cmake --build .
sudo cmake --build . --target install
```

**Installing MongoDB (optional)**

For installation of MongoDB refer to the official [documentation](https://www.mongodb.com/docs/manual/installation).
