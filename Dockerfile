#FROM ubunutu:1.17.2-alpine AS build-env
FROM ubuntu:22.04

RUN apt update && apt install -y git build-essential cmake libgmp-dev libsodium-dev nasm curl m4 nodejs npm
RUN git clone https://github.com/okx/rapidsnark.git
WORKDIR /rapidsnark

RUN npm install
RUN git submodule init
RUN git submodule update
RUN npm run task buildPistache
RUN npm run task createFieldSources
RUN npm run task buildProverServer

ENV LD_LIBRARY_PATH=depends/pistache/build/src

CMD ["/bin/bash"]