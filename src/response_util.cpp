#include "response_util.hpp"
#include <chrono>
#include <ctime>

json ErrorResponse(std::string msg) {
    json err;
    err["code"] = 1;
    err["msg"] = msg;
    
    return err;
}

json SuccessStatus(std::string status) {
    json result;
    result["code"] = 0;
    result["msg"] = "get status success";
    result["status"] = status;

    return result;
}

json SuccessStartPove(std::string proofId) {
    json result;
    result["code"] = 0;
    result["msg"] = "start prove success";
    result["proof_id"] = proofId;

    return result;
}

json SuccessGenerateProof(std::string proofId,json proof,json pubData) {
    json result;
    result["code"] = 0;
    result["proof_id"] = proofId;
    result["proof"] = proof;
    result["pubData"] = pubData;
    return result;
}

int get_time() {
    auto now = std::chrono::system_clock::now();
    std::time_t unix_timestamp = std::chrono::system_clock::to_time_t(now);
    int timestamp_as_int = static_cast<int>(unix_timestamp);
    return timestamp_as_int;
}