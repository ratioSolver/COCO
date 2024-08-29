# Start from the base image
FROM ubuntu:24.04 as builder

# Install dependencies
RUN apt update && apt install -y build-essential cmake git curl unzip

# Install MongoDB cxx drivers
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz
RUN tar -xzf mongo-cxx-driver-r3.10.1.tar.gz
WORKDIR mongo-cxx-driver-r3.10.1/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF
RUN cmake --build .
RUN cmake --build . --target install

# Install CLIPS
RUN curl -OL https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.1/clips_core_source_641.zip
RUN unzip clips_core_source_641.zip
WORKDIR clips_core_source_641/core
RUN make release_cpp
RUN mkdir /usr/local/include/clips
RUN cp *.h /usr/local/include/clips
RUN cp libclips.a /usr/local/lib
