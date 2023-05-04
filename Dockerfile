FROM ubuntu:22.04
WORKDIR /home
RUN apt-get update && apt-get install build-essential libboost-all-dev cmake libssl-dev git wget curl python3 -y

# Install CLIPS
RUN wget -O clips_core_source_641.tar.gz https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.1/clips_core_source_641.tar.gz/download
RUN tar -xvf clips_core_source_641.tar.gz
WORKDIR /home/clips_core_source_641/core
RUN make release_cpp
RUN mkdir /usr/local/include/clips
RUN cp *.h /usr/local/include/clips
RUN cp libclips.a /usr/local/lib

# Install Paho MQTT C/C++ libraries
WORKDIR /home
RUN git clone https://github.com/eclipse/paho.mqtt.c.git
WORKDIR /home/paho.mqtt.c
RUN cmake -Bbuild -H. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON -DPAHO_WITH_SSL=ON -DPAHO_HIGH_PERFORMANCE=ON
RUN cmake --build build/ --target install
RUN ldconfig

WORKDIR /home
RUN git clone https://github.com/eclipse/paho.mqtt.cpp
WORKDIR /home/paho.mqtt.cpp
RUN cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON -DPAHO_BUILD_SAMPLES=TRUE
RUN cmake --build build/ --target install
RUN ldconfig

# Install MongoDB cxx drivers
WORKDIR /home
RUN git clone https://github.com/mongodb/mongo-c-driver.git
WORKDIR /home/mongo-c-driver
RUN python3 build/calc_release_version.py > VERSION_CURRENT
RUN mkdir cmake-build
WORKDIR /home/mongo-c-driver/cmake-build
RUN cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..
RUN cmake --build .
RUN cmake --build . --target install

WORKDIR /home
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.7.0/mongo-cxx-driver-r3.7.0.tar.gz
RUN tar -xzf mongo-cxx-driver-r3.7.0.tar.gz
WORKDIR /home/mongo-cxx-driver-r3.7.0/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
RUN cmake --build . --target EP_mnmlstc_core
RUN cmake --build .
RUN cmake --build . --target install

# Install Dashboard
WORKDIR /home
RUN git clone --recursive https://github.com/CTEMT/Dashboard
WORKDIR /home/Dashboard
RUN mkdir build && cd build && cmake .. && make
COPY ./rules /home/rules
CMD /home/Dashboard/build/bin/Dashboard -rules /home/rules/rules.clp