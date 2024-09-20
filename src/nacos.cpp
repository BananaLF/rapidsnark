#include "nacos.hpp"
#include <algorithm>
#include <cctype>
#include "Nacos.h"

using namespace nacos;
const char* ENABLE_RAPIDSNARK_NACOS="ENABLE_RAPIDSNARK_NACOS";
const char* RAPIDSNARK_NACOS_SERVER="RAPIDSNARK_NACOS_SERVER";
const char* RAPIDSNARK_NACOS_SERVICE_NAME="RAPIDSNARK_NACOS_SERVICE_NAME";
const char* RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME="RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME";
const char* RAPIDSNARK_NACOS_INSTANCE_IP="MY_POD_IP";
const char* RAPIDSNARK_NACOS_INSTANCE_PORT="RAPIDSNARK_NACOS_INSTANCE_PORT";
const char* RAPIDSNARK_NACOS_INSTANCE_ID="RAPIDSNARK_NACOS_INSTANCE_ID";
const char* RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL="RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL";
const char* RAPIDSNARK_NACOS_INSTANCE_NAMESAPCE="RAPIDSNARK_NACOS_INSTANCE_NAMESAPCE";
NacosService::NacosService() {
    const char* envEnableNacos = std::getenv(ENABLE_RAPIDSNARK_NACOS);
    if (envEnableNacos != nullptr) {
        this->enableNacos = stringToBool(envEnableNacos);
    }
    std::cout << "enable nacos: " << this->enableNacos << std::endl;
    if (!this->enableNacos) {
        return;
    }

    const char* envNacosServer = std::getenv(RAPIDSNARK_NACOS_SERVER);
    const char* envNacosSeviceName = std::getenv(RAPIDSNARK_NACOS_SERVICE_NAME);
    const char* envNacosClusterName = std::getenv(RAPIDSNARK_NACOS_INSTANCE_CLUSTER_NAME);
    const char* envNacosInstanceIP = std::getenv(RAPIDSNARK_NACOS_INSTANCE_IP);
    const char* envNacosInstancePort = std::getenv(RAPIDSNARK_NACOS_INSTANCE_PORT);
    const char* envNacosInstanceID = std::getenv(RAPIDSNARK_NACOS_INSTANCE_ID);
    const char* envNacosInstanceEphemeral = std::getenv(RAPIDSNARK_NACOS_INSTANCE_EPHEMERAL);
    const char* envNacosInstanceNamespace = std::getenv(RAPIDSNARK_NACOS_INSTANCE_NAMESAPCE);
    
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
    } else if (envNacosInstanceNamespace == nullptr ) {
        std::cerr << "env "<< RAPIDSNARK_NACOS_INSTANCE_NAMESAPCE << " is empty" << std::endl;
        std::exit(EXIT_FAILURE);
    } 

    this->nacosServer = envNacosServer;
    this->serviceName = envNacosSeviceName;
    this->clusterName = envNacosClusterName;
    this->ip = envNacosInstanceIP;
    this->port= std::stoi(envNacosInstancePort);
    this->instanceId = envNacosInstanceID;
    this->ephemeral = stringToBool(envNacosInstanceEphemeral);
    this->namespaceData = envNacosInstanceNamespace;
}

NacosService::~NacosService() {}

void NacosService::registerInstance() {
    if (!this->enableNacos) {
        return;
    }
    Properties configProps;
    configProps[PropertyKeyConst::SERVER_ADDR] = this->nacosServer;
    configProps[PropertyKeyConst::NAMESPACE] = this->namespaceData;
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
    instance.groupName = "DEFAULT_GROUP";

    NacosString serviceName = this->serviceName;
    std::cerr << "nacosServer:" << this->nacosServer << std::endl;
    std::cerr << "namespaceData:" << this->namespaceData << std::endl;
    std::cerr << "clusterName:" << this->clusterName << std::endl;
    std::cerr << "ip:" << this->ip << std::endl;
    std::cerr << "port:" << this->port << std::endl;
    std::cerr << "instanceId:" << this->instanceId << std::endl;
    std::cerr << "ephemeral:" << this->ephemeral << std::endl;
    std::cerr << "groupName:" << instance.groupName << std::endl;
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