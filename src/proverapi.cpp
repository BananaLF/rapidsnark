#include "proverapi.hpp"
#include "nlohmann/json.hpp"
#include "logger.hpp"
#include "response_util.hpp"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>


using namespace CPlusPlusLogging;
using namespace Pistache;
using json = nlohmann::json;


void ProverAPI::postInput(const Rest::Request& request, Http::ResponseWriter response) {
    std::string circuit(request.param(":circuit").as<std::string>());
    
    int current_time = get_time();
    std::string proofId = "proof-" + std::to_string(current_time);
    std::string log_str = circuit + "-"+ proofId;
    LOG_TRACE(log_str);
    
    json prover_result = fullProver.startProve(request.body(), circuit, proofId);
    response.send(Http::Code::Ok, prover_result.dump(), MIME(Application, Json));
}

void ProverAPI::postCancel(const Rest::Request& request, Http::ResponseWriter response) {
    fullProver.abort();
    response.send(Http::Code::Ok);
}

void ProverAPI::getStatus(const Rest::Request& request, Http::ResponseWriter response) {
    json j = fullProver.getStatus();
    LOG_DEBUG(j.dump().c_str());
    response.send(Http::Code::Ok, j.dump(), MIME(Application, Json));
}

void ProverAPI::postStart(const Rest::Request& request, Http::ResponseWriter response) {
    response.send(Http::Code::Ok);
}

void ProverAPI::postStop(const Rest::Request& request, Http::ResponseWriter response) {
    response.send(Http::Code::Ok);
}

void ProverAPI::getConfig(const Rest::Request& request, Http::ResponseWriter response) {
    response.send(Http::Code::Ok);
}

void ProverAPI::postConfig(const Rest::Request& request, Http::ResponseWriter response) {
    response.send(Http::Code::Ok);
}

void ProverAPI::postGetProof(const Rest::Request& request, Http::ResponseWriter response) {
    std::string proofId(request.param(":proof_id").as<std::string>());
    json result = fullProver.getProof(proofId);
    response.send(Http::Code::Ok, result.dump(), MIME(Application, Json));
}
