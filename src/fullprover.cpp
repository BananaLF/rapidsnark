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
#include "response_util.hpp"
#include "temp_file.hpp"

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
    LOG_INFO("start prove:");
    LOG_INFO(proofId);
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
    LOG_INFO("start prove success: ");
    if (zkHeaders.find(circuit) == zkHeaders.end()) {
        std::string errString = circuit + " is not exist in this prover server";
        return ErrorResponse(errString);    
    }
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
        
        std::ofstream file("/tmp/rapidsnark/build/input_"+ circuit +".json");
        file << j;
        file.close();

        std::string witnessFile("/tmp/rapidsnark/build/" + circuit + ".wtns");
        std::string command("./build/" + circuit + " /tmp/rapidsnark/build/input_"+ circuit +".json " + witnessFile);
        LOG_TRACE(command);
        std::array<char, 128> buffer;
        std::string result;

        // std::cout << "Opening reading pipe" << std::endl;
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe)
        {
            LOG_ERROR("Couldn't start command.");
            throw std::invalid_argument( "Couldn't start witness command." );
        }
        while (fgets(buffer.data(), 128, pipe) != NULL) {
            // std::cout << "Reading..." << std::endl;
            result += buffer.data();
        }
        auto returnCode = pclose(pipe);

        if (returnCode != 0) { 
            std::string background_err = "circuit:" + circuit + " proof_id:" + executingCircuit + " generate witness failed." + 
            "\n returnCode: " + std::to_string(returnCode) + "\n result: " + result;
            LOG_ERROR(background_err);
            std::ostringstream oss;
            oss << "generate witness failed. returnCode: " << returnCode << " result: " << result;
            std::string errMsg= oss.str();
            throw std::invalid_argument(errMsg);
        }
        
        
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
        writ_temp_file(proofResult,executingProofId);

        calcFinished();
    } catch (const std::invalid_argument& e) {  // 捕获 std::invalid_argument
        LOG_TRACE("invalid_argument catch get runtime err");
        if (!isCanceled()) {
            LOG_TRACE("no cancel");
            errString = e.what();
            json proofResult = ErrorGenerateProof(executingProofId,errString);
            writ_temp_file(proofResult,executingProofId);
        }
        LOG_TRACE(e.what());
        LOG_TRACE("catch end");
        calcFinished();
    } catch (std::runtime_error& e) {
        LOG_TRACE("runtime_error catch get runtime err");
        if (!isCanceled()) {
            LOG_TRACE("no cancel");
            errString = e.what();
            json proofResult = ErrorGenerateProof(executingProofId,errString);
            writ_temp_file(proofResult,executingProofId);
        }
        LOG_TRACE(e.what());
        LOG_TRACE("catch end");
        calcFinished();
    } catch (const json::parse_error& e) {
        LOG_TRACE("parse_error catch get runtime err");
        if (!isCanceled()) {
            LOG_TRACE("no cancel");
            errString = e.what();
            json proofResult = ErrorGenerateProof(executingProofId,errString);
            writ_temp_file(proofResult,executingProofId);
        }
        LOG_TRACE(e.what());
        LOG_TRACE("catch end");
        calcFinished();
     } catch (const std::exception& e) {
        LOG_TRACE("exception catch get runtime err");
        if (!isCanceled()) {
            LOG_TRACE("no cancel");
            errString = e.what();
            json proofResult = ErrorGenerateProof(executingProofId,"exception"+errString);
            proofResult["code"] = 2;//must be handle it
            writ_temp_file(proofResult,executingProofId);
        }
        LOG_TRACE(e.what());
        LOG_TRACE("catch end");
        calcFinished();
    } catch (...) {
       LOG_TRACE("exception catch get runtime err");
        if (!isCanceled()) {
            LOG_TRACE("no cancel");
            json proofResult = ErrorGenerateProof(executingProofId,"occur unknown error");
            proofResult["code"] = 2;//must be handle it
            writ_temp_file(proofResult,executingProofId);
        }
        LOG_TRACE("catch end");
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
        st = ErrorGenerateProof(executingProofId,errString);
    } else if (status == success) {
        LOG_TRACE("success");
        st = SuccessGenerateProof(executingProofId,proof,pubData);
        st["status"] = "success";
    } else if (status == busy) {
        LOG_TRACE("busy");
        st = SuccessStatus("busy");
        st["current_proof"] = executingProofId;
    }
    LOG_TRACE("FullProver::getStatus end");
    return st;
}

json FullProver::getProof(std::string proofId) {
    std::ifstream file("/tmp/rapidsnark/build/temp_proof/" + proofId + ".json");

    if (!file.is_open()) {
        return ErrorResponse("can not find proofId: " + proofId);
    }

    json result;
    file >> result; 
    file.close();
    // reduce_temp_proof();
    return result;
}