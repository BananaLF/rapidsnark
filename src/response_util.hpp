#include <nlohmann/json.hpp>
#include <filesystem>


using json = nlohmann::json;

json ErrorResponse(std::string msg);
json SuccessStartPove(std::string proofId);
json SuccessGenerateProof(std::string proofId,json proof,json pubData);
json ErrorGenerateProof(std::string proofId,std::string msg);
int get_time();
json SuccessStatus(std::string status);