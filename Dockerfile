# Use a base image with Ubuntu
FROM ubuntu:latest

# Install the necessary dependencies
RUN apt update && apt install -y build-essential cmake libssl-dev unzip wget

# Compile and install CLIPS
RUN wget -O /tmp/clips.zip https://sourceforge.net/projects/clipsrules/files/CLIPS/6.4.1/clips_core_source_641.zip/download
RUN unzip /tmp/clips.zip -d /tmp
WORKDIR /tmp/clips/clips_core_source_641/core
RUN make release_cpp
RUN mkdir -p /usr/local/include/clips
RUN cp *.h /usr/local/include/clips
RUN cp libclips.a /usr/local/lib

# Compile and install the mongo-cxx driver
WORKDIR /tmp
RUN curl -OL https://github.com/mongodb/mongo-cxx-driver/releases/download/r3.10.1/mongo-cxx-driver-r3.10.1.tar.gz
RUN tar -xzf mongo-cxx-driver-r3.10.1.tar.gz
WORKDIR /tmp/mongo-cxx-driver-r3.10.1/build
RUN cmake .. -DCMAKE_BUILD_TYPE=Release -DMONGOCXX_OVERRIDE_DEFAULT_INSTALL_PREFIX=OFF
RUN cmake --build .
RUN cmake --build . --target install

# Install Node.js through NVM
WORKDIR /tmp
RUN wget -qO- https://raw.githubusercontent.com/nvm-sh/nvm/v0.01.1/install.sh | bash
RUN export NVM_DIR="$([ -z "${XDG_CONFIG_HOME-}" ] && printf %s "${HOME}/.nvm" || printf %s "${XDG_CONFIG_HOME}/nvm")"
RUN [ -s "$NVM_DIR/nvm.sh" ] && \. "$NVM_DIR/nvm.sh"
RUN nvm install node
