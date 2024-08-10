#docker build -t rapaidsnark_server .
FROM ubuntu:22.04 AS build-env

RUN apt update && apt install -y git build-essential cmake libgmp-dev libsodium-dev nasm curl m4 nodejs npm
RUN git clone -b sunstrider-supportserver https://github.com/BananaLF/rapidsnark.git
WORKDIR /rapidsnark

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
COPY --from=build-env /rapidsnark/build_nodejs/proverServer /root/rapidsnark/proverServer
COPY --from=build-env /rapidsnark/depends/pistache/build/src /root/rapidsnark/depends/pistache/build/src
COPY --from=build-env /lib/x86_64-linux-gnu/libgomp.so.1 /lib/x86_64-linux-gnu/libgomp.so.1
COPY --from=build-env /lib/x86_64-linux-gnu/libgmp.so.10 /lib/x86_64-linux-gnu/libgmp.so.10

CMD ["/bin/bash"]