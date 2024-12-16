#include <nlohmann/json.hpp>


using json = nlohmann::json;



json reduce_temp_file();
void writ_temp_file(json data,std::string proofId);
void try_create_temp_dir();