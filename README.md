# CoCo

![Build Status](https://github.com/ratioSolver/COCO/actions/workflows/cmake.yml/badge.svg)
[![codecov](https://codecov.io/gh/ratioSolver/COCO/branch/master/graph/badge.svg)](https://codecov.io/gh/ratioSolver/COCO)

combined deduCtiOn and abduCtiOn (CoCo) reasoner.

## How to use

## Installation of CoCo on a local machine

### CLIPS

CoCo relies on [CLIPS](https://www.clipsrules.net) for reacting to the dynamic changes which happen into the urban environment.
 - Download [CLIPS v6.4.2](https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.1/clips_core_source_641.zip/download) and unzip the zip file into the `clips_core_source_642` folder.
 - Reach the `clips_core_source_642/core` folder and compile CLIPS through `make release_cpp`.
 - Copy all the header files into the `/usr/local/include/clips` folder through `sudo cp *.h /usr/local/include/clips/`.
 - Copy the library into the `/usr/local/lib` folder through `sudo cp libclips.a /usr/local/lib/`.

### MongoDB

CoCo relies on [MongoDB](https://www.mongodb.com) for storing the data. It is required, in particular, to install the [cxx drivers](https://www.mongodb.com/docs/drivers/cxx/) for connecting CoCo to a MongoDB database.

**Installig OpenSSL**

```shell
sudo apt-get install libssl-dev
```

**Installing MongoDB cxx drivers**

Download and configure the mongo-cxx driver.

```shell
curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz
tar -xzf mongo-cxx-driver-r3.10.1.tar.gz
cd mongo-cxx-driver-r3.10.1/build
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

### MQTT Integration (optional)

CoCo allows to integrate with MQTT brokers for receiving data from IoT devices. To enable this feature, it is required to install the [Paho MQTT C++ library](https://github.com/eclipse/paho.mqtt.cpp).

**Installing Paho MQTT C++ library**

```shell
sudo apt-get install libssl-dev
```

Download and configure the Paho MQTT C++ library.

```shell
git clone https://github.com/eclipse/paho.mqtt.cpp
cd paho.mqtt.cpp
git checkout v1.4.0
git submodule init
git submodule update

cmake -Bbuild -H. -DPAHO_WITH_MQTT_C=ON
sudo cmake --build build/ --target install
```