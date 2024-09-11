#include <iostream>


using namespace std;


class NacosService {
    std::string nacosServer;
    std::string serviceName;
    std::string clusterName;
    std::string ip;
    int port;
    std::string instanceId;
    bool ephemeral = true;

 public:
    NacosService();
    ~NacosService();
    void registerInstance();  
};

bool stringToBool(const std::string& str);