#include <iostream>


using namespace std;


class NacosService {
    bool enableNacos = false;
    std::string nacosServer;
    std::string serviceName;
    std::string clusterName;
    std::string ip;
    int port;
    std::string instanceId;
    std::string namespaceData;
    bool ephemeral = true;

 public:
    NacosService();
    ~NacosService();
    void registerInstance();  
};

bool stringToBool(const std::string& str);