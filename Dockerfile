#docker build -t rapaidsnark_server .
FROM zk_email_base:0.1 AS build-env

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

ENV LD_LIBRARY_PATH=/root/rapidsnark/depends/pistache/build/src
RUN mkdir -p /root/rapidsnark/build
COPY --from=build-env /rapidsnark/build_nodejs/proverServer /root/rapidsnark/proverServer
COPY --from=build-env /rapidsnark/depends/pistache/build/src /root/rapidsnark/depends/pistache/build/src
COPY --from=build-env /lib/x86_64-linux-gnu/libgomp.so.1 /lib/x86_64-linux-gnu/libgomp.so.1
COPY --from=build-env /lib/x86_64-linux-gnu/libgmp.so.10 /lib/x86_64-linux-gnu/libgmp.so.10

CMD ["/bin/bash"]