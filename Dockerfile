# Use a base image with Ubuntu
FROM ubuntu:latest

# Install the necessary dependencies
RUN apt update && apt install -y build-essential cmake libssl-dev unzip wget curl git

# Compile and install CLIPS
RUN wget -O /tmp/clips.zip https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.2/clips_core_source_642.zip/download \
    && unzip /tmp/clips.zip -d /tmp \
    && cd /tmp/clips_core_source_642/core \
    && make release_cpp \
    && mkdir -p /usr/local/include/clips \
    && cp *.h /usr/local/include/clips \
    && cp libclips.a /usr/local/lib \
    && rm -rf /tmp/clips.zip /tmp/clips_core_source_642

# Compile and install the mongo-cxx driver
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.1.2/mongo-cxx-driver-r4.1.2.tar.gz \
    && tar -xzf mongo-cxx-driver-r4.1.2.tar.gz \
    && cd mongo-cxx-driver-r4.1.2/build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 \
    && cmake --build . \
    && cmake --build . --target install \
    && cd /tmp && rm -rf mongo-cxx-driver-r4.1.2* \
    && ldconfig

# Compile and install the Paho MQTT C++ library
RUN git clone https://github.com/eclipse/paho.mqtt.cpp.git /tmp/paho.mqtt.cpp \
    && cd /tmp/paho.mqtt.cpp \
    && git submodule update --init \
    && cmake -Bbuild -H. -DPAHO_WITH_MQTT_C=ON \
    && cmake --build build/ --target install \
    && rm -rf /tmp/paho.mqtt.cpp

# Install Node.js (version 22.x)
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y nodejs
