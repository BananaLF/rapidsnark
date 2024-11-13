#include "temp_file.hpp"
#include "response_util.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

const std::string TEMP_FILE_DIR = "/temp/rapidsnark/build/temp_proof/";

json reduce_temp_file() {
    try {
        // check directory is exist
        if (!fs::exists(TEMP_FILE_DIR) || !fs::is_directory(TEMP_FILE_DIR)) {
             return ErrorResponse("prover server is wrong,beacause directory /temp/rapidsnark/build/temp_proof is not exist");
        }
        std::vector<std::string> filenames;
        for (const auto& entry : fs::recursive_directory_iterator(TEMP_FILE_DIR)) {
            if (fs::is_regular_file(entry.path())) {
                filenames.push_back(entry.path().string());
            }
        }
        std::sort(filenames.begin(), filenames.end());
        if (filenames.size() > 10) {
            fs::remove(filenames.at(0));
        }
    } catch (const fs::filesystem_error& e) {
        std::string error_message = e.what();
        std::string errString = "prover server is wrong,beacause directory. Filesystem error: " + error_message;
        return ErrorResponse(errString);
    }

    return SuccessStatus("reduce file success");
}

void writ_temp_file(json data, std::string proofId) {
    std::string filename = TEMP_FILE_DIR + proofId + ".json";
    std::ofstream proofResultFile(filename);
    proofResultFile << data.dump(4);
    proofResultFile.close();
}


void try_create_temp_dir() {
    try {
        // create /temp/rapidsnark/build/temp_proof
        if (fs::create_directory(TEMP_FILE_DIR)) {
            std::cout << "Directory /temp/rapidsnark/build/temp_proof created successfully: " << TEMP_FILE_DIR << std::endl;
        } else {
            std::cout << "Directory /temp/rapidsnark/build/temp_proof already exists " << TEMP_FILE_DIR << std::endl;
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "try_create_temp_dir Error: " << e.what() << std::endl;
    }
}
