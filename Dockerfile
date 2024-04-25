# Use a base image with Ubuntu
FROM ubuntu:latest

# Install the necessary dependencies
RUN apt update && apt install -y build-essential cmake libboost-all-dev libssl-dev unzip wget

# Compile and install CLIPS
RUN wget -O /tmp/clips.zip https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.1/clips_core_source_641.zip/download
RUN unzip /tmp/clips.zip -d /tmp
WORKDIR /tmp/clips/clips_core_source_641/core
RUN make release_cpp
RUN mkdir -p /usr/local/include/clips
RUN cp *.h /usr/local/include/clips
RUN cp libclips.a /usr/local/lib

# Compile and install the mongo-c driver
RUN git clone https://github.com/mongodb/mongo-c-driver.git /tmp/mongo-c-driver
WORKDIR /tmp/mongo-c-driver
RUN python3 build/calc_release_version.py > VERSION_CURRENT
RUN mkdir cmake-build
WORKDIR /tmp/mongo-c-driver/cmake-build
RUN cmake -DENABLE_EXTRA_ALIGNMENT=OFF -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF ..

# Compile and install the mongo-cxx driver
WORKDIR /tmp
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz
RUN tar -xzf mongo-cxx-driver-r3.10.1.tar.gz
WORKDIR /tmp/mongo-cxx-driver-r3.10.1/build
RUN cmake -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF ..
RUN cmake --build . --target install

# Clean up the temporary files
WORKDIR /
RUN rm -rf /tmp
