FROM office-registry-vpc.cn-hongkong.cr.aliyuncs.com/okbase/node:20.11.1-okg2
WORKDIR /root/rapidsnark

RUN apt-get update && apt-get install -y libcurl4-openssl-dev zlib1g-dev
RUN mkdir -p /root/rapidsnark/build
RUN mkdir -p /tmp/rapidsnark/build
RUN pwd
RUN ls
COPY ./artifacts/proverServer /root/rapidsnark/proverServer
COPY ./artifacts/libnacos-cli.so /usr/local/lib/libnacos-cli.so
COPY ./artifacts/libpistache.so.0 /usr/local/lib/libpistache.so.0
COPY ./artifacts/libgomp.so.1 /lib/x86_64-linux-gnu/libgomp.so.1
COPY ./artifacts/libgmp.so.10 /lib/x86_64-linux-gnu/libgmp.so.10

CMD ["/bin/bash"]