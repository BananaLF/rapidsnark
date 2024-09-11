#include "nacos.hpp"
#include <algorithm>
#include <cctype>
#include "Nacos.h"

using namespace nacos;
const char* RAPIDSNARK_NACOS_SERVER="RAPIDSNARK_NACOS_SERVER";
const char* RAPIDSNARK_NACOS_SERVICE_NAME="RAPIDSNARK_NACOS_SERVICE_NAME";
const char* RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME="RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME";
const char* RAPIDSNARK_NACOS_INSTANCE_IP="MY_POD_IP";
const char* RAPIDSNARK_NACOS_INSTANCE_PORT="RAPIDSNARK_NACOS_INSTANCE_PORT";
const char* RAPIDSNARK_NACOS_INSTANCE_ID="RAPIDSNARK_NACOS_INSTANCE_ID";
const char* RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL="RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL";
NacosService::NacosService() {
    const char* envNacosServer = std::getenv(RAPIDSNARK_NACOS_SERVER);
    const char* envNacosSeviceName = std::getenv(RAPIDSNARK_NACOS_SERVICE_NAME);
    const char* envNacosClusterName = std::getenv(RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME);
    const char* envNacosInstanceIP = std::getenv(RAPIDSNARK_NACOS_INSTANCE_IP);
    const char* envNacosInstancePort = std::getenv(RAPIDSNARK_NACOS_INSTANCE_PORT);
    const char* envNacosInstanceID = std::getenv(RAPIDSNARK_NACOS_INSTANCE_ID);
    const char* envNacosInstanceEphemeral = std::getenv(RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL);
    
    if (envNacosServer == nullptr) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_SERVER << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosSeviceName == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_SERVICE_NAME << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosClusterName == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosInstanceIP == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_IP << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosInstancePort == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_PORT << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosInstanceID == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_ID << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } else if (envNacosInstanceEphemeral == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } 

    this->nacosServer = envNacosServer;
    this->serviceName = envNacosSeviceName;
    this->clusterName = envNacosClusterName;
    this->ip = envNacosInstanceIP;
    this->port= std::stoi(envNacosInstancePort);
    this->instanceId = envNacosInstanceID;
    this->ephemeral = stringToBool(envNacosInstanceEphemeral);
}

NacosService::~NacosService() {}

void NacosService::registerInstance() {
    Properties configProps;
    configProps[PropertyKeyConst::SERVER_ADDR] = this->nacosServer;
    INacosServiceFactory *factory = NacosFactoryFactory::getNacosFactory(configProps);
    ResourceGuard <INacosServiceFactory> _guardFactory(factory);
    NamingService *namingSvc = factory->CreateNamingService();
    ResourceGuard <NamingService> _serviceFactory(namingSvc);
    Instance instance;
    instance.clusterName = this->clusterName;
    instance.ip = this->ip;
    instance.port = this->port;
    instance.instanceId = this->instanceId;
    instance.ephemeral = this->ephemeral;

    NacosString serviceName = this->serviceName;
    namingSvc->registerInstance(serviceName, instance);
}

bool stringToBool(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (s == "true" || s == "1") {
        return true;
    } else if (s == "false" || s == "0") {
        return false;
    } else {
        throw std::invalid_argument("Invalid boolean string");
    }
}