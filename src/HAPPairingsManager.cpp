#include "HomeKitAccessory.h"
#include "network.h"
#include "tlv.h"
#include "hap_crypto.h"

#include <cstring>

HAPPairingsManager::HAPPairingsManager(HAPServer * server): server(server) {
    //Add crypto callbacks to server
    hap_crypto_init(server);
}

void HAPPairingsManager::onPairSetup(HAPUserHelper * request) {
    auto sendError = [&request](tlv8_error_code error, int status = HTTP_400_BAD_REQUEST){
        auto state = request->pairInfo()->currentStep;
        auto res = tlv8_insert(nullptr, kTLVType_Error, 1, &error);
        tlv8_insert(res, kTLVType_State, 1, &state);
        request->setResponseStatus(status);
        request->send(res);
    };

    if(request->requestContentType() != HAP_PAIRING_TLV8){
        sendError(kTLVError_Unknown);
        return;
    }

    auto pairInfo = request->pairInfo();
    auto& setupStore = pairInfo->setupStore;
    auto chain = tlv8_parse(request->data(), request->dataLength());
    HAP_DEBUG("pairing request: state(%d)",
              *tlv8_find(chain, kTLVType_State)->value);

    auto requestPairStep = *tlv8_find(chain, kTLVType_State)->value;

    if(pairInfo->isPaired){
        HAP_DEBUG("Weird...client is already paired, but it requests to pair again.");
        sendError(kTLVError_Unknown);
        return;
    }

    if(pairInfo->currentStep + 1 != requestPairStep || (!pairInfo->isPairing && requestPairStep > 1)){
        HAP_DEBUG("HAP Client tries to skip step. Warned.");
        sendError(kTLVError_Authentication);
        return;
    }

    if(!pairInfo->isPairing && requestPairStep == 1){
        pairInfo->isPairing = true;
        pairInfo->currentStep = 0;

        delete pairInfo->setupStore;
        pairInfo->setupStore = new hap_crypto_setup(server, "Pair-Setup", server->setupCode);
    }

    setupStore->session = request;
    switch (requestPairStep){
        case 1: //M1
            request->retain(); //complimentary release in HAPPairingsManager::onPairSetupM2Finish
            hap_crypto_srp_init(setupStore);
            pairInfo->currentStep = 2;
            break;
        case 3: { //M3
            auto ATlv = tlv8_find(chain, kTLVType_PublicKey);
            auto ALen = tlv8_value_length(ATlv);
            auto A = new uint8_t[ALen];
            tlv8_read(ATlv, A, ALen);
            setupStore->A = A;
            setupStore->ALen = static_cast<uint16_t>(ALen);

            auto MTlv = tlv8_find(chain, kTLVType_Proof);
            auto MLen = tlv8_value_length(MTlv);
            auto M = new uint8_t[MLen];
            tlv8_read(MTlv, M, MLen);
            setupStore->clientProof = M;
            //We don't store the length since we know its 512bits

            request->retain(); //release in HAPPairingsManager::onPairSetupM4Finish
            hap_crypto_srp_proof(setupStore);
            pairInfo->currentStep = 4;
            break;
        }
        default:
            HAP_DEBUG("Unknown pairing step %u", requestPairStep);
    }

    tlv8_free(chain);
//    request->close();
}

void HAPPairingsManager::onPairSetupM2Finish(hap_crypto_setup * info) {
    auto request = info->session;
    info->session = nullptr;

    uint8_t M2 = 2;

    auto response = tlv8_insert(nullptr, kTLVType_Salt, 16, info->salt);
    response = tlv8_insert(response, kTLVType_PublicKey, info->BLen, info->B);
    response = tlv8_insert(response, kTLVType_State, 1, &M2);

    request->send(response);
    request->release();
}

void HAPPairingsManager::onPairSetupM4Finish(hap_crypto_setup * info) {
    auto request = info->session;
    info->session = nullptr;

    tlv8_item * response = nullptr;

    if(hap_crypto_verify_client_proof(info)){
        response = tlv8_insert(response, kTLVType_Proof, 64, info->serverProof);
    } else {
        uint8_t error = kTLVError_Authentication;
        response = tlv8_insert(response, kTLVType_Error, 1, &error);
    }

    uint8_t M4 = 4;
    response = tlv8_insert(response, kTLVType_State, 1, &M4);
    request->send(response);
    request->release();
}

hap_pair_info::~hap_pair_info() {
    delete setupStore;
}
