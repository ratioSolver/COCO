# Use a base image with Ubuntu
FROM ubuntu:latest

# Expose the COCO application port
EXPOSE 8080

# Install the necessary dependencies
RUN apt update && apt install -y \
    build-essential cmake libssl-dev unzip wget curl git ca-certificates \
    && rm -rf /var/lib/apt/lists/*

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
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r4.1.1/mongo-cxx-driver-r4.1.1.tar.gz \
    && tar -xzf mongo-cxx-driver-r4.1.1.tar.gz \
    && cd mongo-cxx-driver-r4.1.1/build \
    && cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=17 \
    && cmake --build . \
    && cmake --build . --target install \
    && cd /tmp && rm -rf mongo-cxx-driver-r4.1.1* \
    && ldconfig

# Install Node.js through NVM
RUN curl -fsSL https://deb.nodesource.com/setup_22.x | bash - \
    && apt-get install -y nodejs

# Clone and build COCO
WORKDIR /app
RUN git clone --recursive -b uncertainty https://github.com/ratioSolver/COCO \
    && cd COCO \
    && mkdir build && cd build \
    && cmake -DLOGGING_LEVEL=DEBUG -DBUILD_MONGODB=ON -DBUILD_COCO_SERVER=ON -DBUILD_AUTH=ON -DBUILD_WEB_APP=ON -DBUILD_COCO_EXECUTABLE=ON -DCMAKE_BUILD_TYPE=Release .. \
    && make

# Start the COCO application
WORKDIR /app/COCO/build
CMD ["./CoCoService"]