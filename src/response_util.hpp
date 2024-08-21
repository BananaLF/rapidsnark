#include <nlohmann/json.hpp>
#include <filesystem>


using json = nlohmann::json;

json ErrorResponse(std::string msg);
json SuccessStartPove(std::string proofId);
json SuccessGenerateProof(std::string proofId,json proof,json pubData);
int get_time();
json SuccessStatus(std::string status);