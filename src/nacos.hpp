#include <iostream>
#include "Nacos.h"


using namespace std;


class NacosService {
    std::shared_ptr<NamingService> namingSvc;
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