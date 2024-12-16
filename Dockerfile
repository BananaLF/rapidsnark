#docker build -t rapaidsnark_server .
FROM ubuntu:22.04 AS build-env

RUN apt-get update && apt-get install -y git build-essential cmake libgmp-dev libsodium-dev nasm curl m4 sudo libcurl4-openssl-dev zlib1g-dev
RUN curl -sL https://deb.nodesource.com/setup_20.x | sudo -E bash - && apt-get install -y nodejs    
# generate nacos
RUN git clone -b v1.1.0 https://github.com/nacos-group/nacos-sdk-cpp.git && cd nacos-sdk-cpp && cmake . && make && make install

WORKDIR /rapidsnark
COPY ./build ./build
COPY ./.git ./.git
COPY ./cmake ./cmake
COPY ./service ./service
COPY ./src ./src
COPY ./testdata ./testdata
COPY ./tools ./tools
COPY ./build_gmp.sh ./build_gmp.sh
COPY ./CMakeLists.txt ./CMakeLists.txt
COPY ./Makefile ./Makefile
COPY ./package-lock.json ./package-lock.json
COPY ./package.json ./package.json
COPY ./tasksfile.js ./tasksfile.js
COPY ./.gitmodules ./.gitmodules

RUN npm install
RUN git submodule init
RUN git submodule update
RUN npm run task buildPistache
RUN npm run task createFieldSources
RUN npm run task buildProverServer

#Final image
FROM ubuntu:22.04
WORKDIR /root/rapidsnark

RUN apt-get update && apt-get install -y libcurl4-openssl-dev zlib1g-dev
RUN mkdir -p /root/rapidsnark/build
COPY --from=build-env /rapidsnark/build_nodejs/proverServer /root/rapidsnark/proverServer
COPY --from=build-env /usr/local/lib/libnacos-cli.so /usr/local/lib/libnacos-cli.so
COPY --from=build-env /usr/local/lib/libpistache.so.0 /usr/local/lib/libpistache.so.0
COPY --from=build-env /lib/x86_64-linux-gnu/libgomp.so.1 /lib/x86_64-linux-gnu/libgomp.so.1
COPY --from=build-env /lib/x86_64-linux-gnu/libgmp.so.10 /lib/x86_64-linux-gnu/libgmp.so.10

CMD ["/bin/bash"]