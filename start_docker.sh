# build docker if need
# docker build -t rapaidsnark_server .
docker run -it \
  -v ./zk-email-testdata:/root/rapidsnark/zk-email-testdata \
  -v ./zk-email-testdata/build-one/demo-zk-email-one_cpp/:/root/rapidsnark/build \
  -p 8080:8080 \
  rapaidsnark_server \
  ./proverServer 8080 /root/rapidsnark/zk-email-testdata/build-one/demo-zk-email-one.zkey
