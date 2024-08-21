#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <filesystem>

#include "fullprover.hpp"
#include "fr.hpp"

#include "logger.hpp"
#include "wtns_utils.hpp"

using namespace CPlusPlusLogging;
namespace fs = std::filesystem;

std::string getfilename(std::string path)
{
    path = path.substr(path.find_last_of("/\\") + 1);
    size_t dot_i = path.find_last_of('.');
    return path.substr(0, dot_i);
}

FullProver::FullProver(std::string zkeyFileNames[], int size) {
    pendingInput="";
    pendingCircuit="";
    canceled = false;

    mpz_init(altBbn128r);
    mpz_set_str(altBbn128r, "21888242871839275222246405745257275088548364400416034343698204186575808495617", 10);

    for(int i = 0; i < size; i++) {
        std::string circuit = getfilename(zkeyFileNames[i]);
        zKeys[circuit] = BinFileUtils::openExisting(zkeyFileNames[i], "zkey", 1);
        zkHeaders[circuit] = ZKeyUtils::loadHeader(zKeys[circuit].get());

        std::string proofStr;
        if (mpz_cmp(zkHeaders[circuit]->rPrime, altBbn128r) != 0) {
            throw std::invalid_argument( "zkey curve not supported" );
        }
        
        std::ostringstream ss1;
        ss1 << "circuit: " << circuit;
        LOG_DEBUG(ss1);

        provers[circuit] = Groth16::makeProver<AltBn128::Engine>(
            zkHeaders[circuit]->nVars,
            zkHeaders[circuit]->nPublic,
            zkHeaders[circuit]->domainSize,
            zkHeaders[circuit]->nCoefs,
            zkHeaders[circuit]->vk_alpha1,
            zkHeaders[circuit]->vk_beta1,
            zkHeaders[circuit]->vk_beta2,
            zkHeaders[circuit]->vk_delta1,
            zkHeaders[circuit]->vk_delta2,
            zKeys[circuit]->getSectionData(4),    // Coefs
            zKeys[circuit]->getSectionData(5),    // pointsA
            zKeys[circuit]->getSectionData(6),    // pointsB1
            zKeys[circuit]->getSectionData(7),    // pointsB2
            zKeys[circuit]->getSectionData(8),    // pointsC
            zKeys[circuit]->getSectionData(9)     // pointsH1
        );
    }

    status = ready;
}

FullProver::~FullProver() {
    mpz_clear(altBbn128r);
}

json FullProver::startProve(std::string input, std::string circuit, std::string proofId) {
    LOG_TRACE("FullProver::startProve begin");
    LOG_DEBUG(input);
    std::lock_guard<std::mutex> guard(mtx);
    LOG_INFO("start prove");
    if (status == busy) {
        LOG_INFO("start prov busy");
        return ErrorResponse("prover is busy");
    }
    json reduceResult = reduce_temp_file();
    if (reduceResult["code"] == 1) {
        LOG_ERROR("reduce_temp_file failed");
        std::string errString = reduceResult.dump(4);
        LOG_ERROR(errString);
        return reduceResult;
    }
    LOG_INFO("start prove success");
    pendingInput = input;
    pendingCircuit = circuit;
    json result = checkPending(proofId);
    LOG_TRACE("FullProver::startProve end");
    return result;
}

json FullProver::checkPending(std::string proofId) {
    LOG_TRACE("FullProver::checkPending begin");
    json result;
    if (status != busy) {
        std::string input = pendingInput;
        std::string circuit = pendingCircuit;
        if (input != "" && circuit != "") {
            status = busy;
            executingInput = pendingInput;
            executingCircuit = pendingCircuit;
            pendingInput = "";
            pendingCircuit = "";
            errString = "";
            executingProofId = proofId;
            canceled = false;
            proof = nlohmann::detail::value_t::null;
            std::thread th(&FullProver::thread_calculateProve, this);
            th.detach();
            result = SuccessStartPove(proofId);
            LOG_TRACE("FullProver::checkPending end");
        } else {
            LOG_TRACE("FullProver::checkPending end");
            result = ErrorResponse("input and circuit is empty");
        }
    } else {
        LOG_TRACE("FullProver::checkPending end");
        result = ErrorResponse("checkPending prover is busy");
    }
    return result;
}

void FullProver::thread_calculateProve() {
    LOG_TRACE("FullProver::thread_calculateProve start");
    
    try {
        LOG_TRACE(executingInput);
        // Generate witness
        json j = json::parse(executingInput);
        std::string circuit = executingCircuit;
        
        std::ofstream file("./build/input_"+ circuit +".json");
        file << j;
        file.close();

        std::string witnessFile("./build/" + circuit + ".wtns");
        std::string command("./build/" + circuit + " ./build/input_"+ circuit +".json " + witnessFile);
        LOG_TRACE(command);
        std::array<char, 128> buffer;
        std::string result;

        // std::cout << "Opening reading pipe" << std::endl;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            std::cerr << "Couldn't start command." << std::endl;
        }
        while (fgets(buffer.data(), 128, pipe) != NULL) {
            // std::cout << "Reading..." << std::endl;
            result += buffer.data();
        }
        auto returnCode = pclose(pipe);

        std::cout << result << std::endl;
        std::cout << returnCode << std::endl;
        // Load witness
        auto wtns = BinFileUtils::openExisting(witnessFile, "wtns", 2);
        auto wtnsHeader = WtnsUtils::loadHeader(wtns.get());
                
        if (mpz_cmp(wtnsHeader->prime, altBbn128r) != 0) {
            throw std::invalid_argument( "different wtns curve" );
        }

        AltBn128::FrElement *wtnsData = (AltBn128::FrElement *)wtns->getSectionData(2);

        pubData.clear();
        AltBn128::FrElement aux;
        for (int i=1; i<=zkHeaders[circuit]->nPublic; i++) {
            AltBn128::Fr.toMontgomery(aux, wtnsData[i]);
            pubData.push_back(AltBn128::Fr.toString(aux));
        }
        
        if (!isCanceled()) {
            proof = provers[circuit]->prove(wtnsData)->toJson();
        } else {
            LOG_TRACE("AVOIDING prove");
            proof = {};
        }
       

        // wirte proofResult.json
        json proofResult = SuccessGenerateProof(executingProofId,proof,pubData);   
        std::ofstream proofResultFile("./build/temp_proof/" + executingProofId + ".json");
        proofResultFile << proofResult.dump(4);
        proofResultFile.close();

        calcFinished();
    } catch (std::runtime_error e) {
        if (!isCanceled()) {
            errString = e.what();
        }
        calcFinished();
    } 

    LOG_TRACE("FullProver::thread_calculateProve end");
}


void FullProver::calcFinished() {
    std::lock_guard<std::mutex> guard(mtx);
    LOG_TRACE("FullProver::calcFinished start");
    if (canceled) {
        LOG_TRACE("FullProver::calcFinished aborted");
        status = aborted;
    } else if (errString != "") {
        LOG_TRACE("FullProver::calcFinished failed");
        status = failed;
    } else {
        LOG_TRACE("FullProver::calcFinished success");
        status = success;
    }
    canceled = false;
    executingInput = "";
    checkPending(executingProofId);
    LOG_TRACE("FullProver::calcFinished end");
}


bool FullProver::isCanceled() {
    std::lock_guard<std::mutex> guard(mtx);
    LOG_TRACE("FullProver::isCanceled start");
    if (canceled) {
        LOG_TRACE("FullProver::isCanceled canceled==true");
    }
    LOG_TRACE("FullProver::isCanceled end");
    return canceled;
}

void FullProver::abort() {
    std::lock_guard<std::mutex> guard(mtx);
    LOG_TRACE("FullProver::abort start");
    if (status!= busy) {
        LOG_TRACE("FullProver::abort end -> not usy");
        return;
    }
    canceled = true;
    LOG_TRACE("FullProver::abort end -> canceled=true");
}


json FullProver::getStatus() {
    LOG_TRACE("FullProver::getStatus start");
    json st;
    if (status == ready) {
        LOG_TRACE("ready");
        st = SuccessStatus("ready");
    } else if (status == aborted) {
        LOG_TRACE("aborted");
        st = SuccessStatus("aborted");
    } else if (status == failed) {
        LOG_TRACE("failed");
        st = ErrorResponse(errString);
    } else if (status == success) {
        LOG_TRACE("success");
        st = SuccessGenerateProof(executingProofId,proof,pubData);
        st["status"] = "success";
    } else if (status == busy) {
        LOG_TRACE("busy");
        st = SuccessStatus("busy");
    }
    LOG_TRACE("FullProver::getStatus end");
    return st;
}

json FullProver::getProof(std::string proofId) {
    std::ifstream file("./build/temp_proof/" + proofId + ".json");

    if (!file.is_open()) {
        return ErrorResponse("can not find proofId: " + proofId);
    }

    json result;
    file >> result; 
    file.close();
    // reduce_temp_proof();
    return result;
}
 
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

json reduce_temp_file() {
    std::string path = "./build/temp_proof/"; 

    try {
        // check directory is exist
        if (!fs::exists(path) || !fs::is_directory(path)) {
             return ErrorResponse("prover server is wrong,beacause directory ./build/temp_proof is not exist");
        }
        LOG_INFO("reduce_temp_proof");
        std::vector<std::string> filenames;
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
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