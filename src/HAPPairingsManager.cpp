#include "HomeKitAccessory.h"
#include "HAPPersistingStorage.h"
#include "network.h"
#include "tlv.h"
#include "hap_crypto.h"

#include <cstring>

#define IOS_DEVICE_X_LEN    100
#define ACCESSORY_X_LEN     81
#define DEVICE_ID_LEN       17

HAPPairingsManager::HAPPairingsManager(HAPServer * server): server(server) {
    //Add crypto callbacks to server
    hap_crypto_init(server);

    //If no accessory keypair, then init one
    if(!server->storage->haveAccessoryLongTermKeys()){
        server->emit(HAPEvent::HAP_INITIALIZE_KEYPAIR);
    }
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
        case 5: { //M5
            const constexpr static uint8_t Msg5Nonce[] = "PS-Msg05";
            const constexpr static unsigned int Msg5NonceLen = sizeof(Msg5Nonce) - 1;
            const constexpr static char hkdfSalt[] = "Pair-Setup-Encrypt-Salt";
            const constexpr static char hkdfInfo[] = "Pair-Setup-Encrypt-Info";

            auto dataTlv = tlv8_find(chain, kTLVType_EncryptedData);
            auto dataTagLen = tlv8_value_length(dataTlv);

            if(dataTagLen > 16){
                auto data = new uint8_t[dataTagLen];
                tlv8_read(dataTlv, data, dataTagLen);
                pairInfo->renewInfoStore(request);

                auto store = pairInfo->infoStore;
                store->encryptedData = data;
                store->dataLen = dataTagLen - 16;
                store->authTag = data + store->dataLen;

                store->nonce = Msg5Nonce;
                store->nonceLen = Msg5NonceLen;
                store->aad = nullptr;
                store->aadLen = 0;

                hap_crypto_derive_key(const_cast<uint8_t *>(store->key),
                                      setupStore->sessionKey, hkdfSalt, hkdfInfo);

                pairInfo->currentStep = 6;
                //If succeed, release in HAPPairingsManager::onPairingDeviceEncryption
                //If decryption failed, release in HAPPairingsManager::onPairingDeviceDecryption
                request->retain();
                hap_crypto_data_decrypt(pairInfo->infoStore);
            }

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
        response = tlv8_insert(response, kTLVType_Proof, HAPCRYPTO_SHA_SIZE, info->serverProof);
    } else {
        uint8_t error = kTLVError_Authentication;
        response = tlv8_insert(response, kTLVType_Error, 1, &error);
    }

    uint8_t M4 = 4;
    response = tlv8_insert(response, kTLVType_State, 1, &M4);
    request->send(response);
    request->release();
}

void HAPPairingsManager::onPairingDeviceDecryption(hap_pair_info * info, HAPUserHelper * request) {
    if(info->currentStep != 6) return;

    if(hap_crypto_data_decrypt_did_succeed(info->infoStore)){
        //Derive iOSDeviceX
        const constexpr static char hkdfSalt[] = "Pair-Setup-Controller-Sign-Salt";
        const constexpr static char hkdfInfo[] = "Pair-Setup-Controller-Sign-Info";

        auto chain = tlv8_parse(info->infoStore->rawData, info->infoStore->dataLen);
        auto deviceLtpk = new uint8_t[32];
        auto signature = new uint8_t[64];
        info->setupStore->deviceLtpk = deviceLtpk;
        tlv8_read(tlv8_find(chain, kTLVType_PublicKey), deviceLtpk, 32);
        tlv8_read(tlv8_find(chain, kTLVType_Signature), signature, 64);

        //iOSDeviceInfo = iOSDeviceX, iOSDevicePairingID, iOSDeviceLTPK
        auto iOSDeviceInfo = new uint8_t[IOS_DEVICE_X_LEN];
        hap_crypto_derive_key(iOSDeviceInfo, info->setupStore->sessionKey, hkdfSalt, hkdfInfo);
        tlv8_read(tlv8_find(chain, kTLVType_Identifier), iOSDeviceInfo + 32, 36);
        memcpy(iOSDeviceInfo + 32 + 36, deviceLtpk, 32);

        tlv8_free(chain);

        if(hap_crypto_verify(signature, iOSDeviceInfo, IOS_DEVICE_X_LEN, deviceLtpk)){
            delete[] signature;
            delete[] iOSDeviceInfo;
            //Mark device as paired
            info->isPaired = true;
            server->emit(HAPEvent::HAP_DEVICE_PAIR, info);
            return;
        }

        delete[] signature;
        delete[] iOSDeviceInfo;
    }

    uint8_t errAuth = kTLVError_Authentication;
    auto resTlv = tlv8_insert(nullptr, kTLVType_Error, 1, &errAuth);
    request->setResponseStatus(400);
    request->send(resTlv);
    request->release();
}

void HAPPairingsManager::onDevicePaired(hap_pair_info * info, HAPUserHelper * request) {
    //Derive AccessoryX
    const constexpr static char hkdfSalt[] = "Pair-Setup-Accessory-Sign-Salt";
    const constexpr static char hkdfInfo[] = "Pair-Setup-Accessory-Sign-Info";
    const constexpr static uint8_t Msg6Nonce[] = "PS-Msg06";
    const constexpr static unsigned int Msg6NonceLen = sizeof(Msg6Nonce) - 1;

    auto crypto = info->infoStore;
    auto pubKey = new uint8_t[32];
    auto secKey = new uint8_t[32];
    server->storage->getAccessoryLongTermKeys(pubKey, secKey);

    //AccessoryInfo = AccessoryX, AccessoryPairingID, AccessoryLTPK
    auto AccessoryInfo = new uint8_t[ACCESSORY_X_LEN];
    hap_crypto_derive_key(AccessoryInfo, info->setupStore->sessionKey, hkdfSalt, hkdfInfo);
    memcpy(AccessoryInfo + 32, server->deviceId, DEVICE_ID_LEN);
    memcpy(AccessoryInfo + 32 + DEVICE_ID_LEN, pubKey, 32);

    auto signature = hap_crypto_sign(AccessoryInfo, ACCESSORY_X_LEN, pubKey, secKey);

    delete[] AccessoryInfo;
    delete[] secKey;

    auto subtlv = tlv8_insert(nullptr, kTLVType_Signature, 64, signature);
    subtlv = tlv8_insert(subtlv, kTLVType_PublicKey, 32, pubKey);
    subtlv = tlv8_insert(subtlv, kTLVType_Identifier, DEVICE_ID_LEN, server->deviceId);

    delete[] pubKey;

    unsigned int subtlvLen = 0;
    auto exportedSubtlv = tlv8_export_free(subtlv, &subtlvLen);

    //Delete original data w/out deleting key
    crypto->reset();
    crypto->rawData = exportedSubtlv;
    crypto->dataLen = subtlvLen;
    crypto->nonce = Msg6Nonce;
    crypto->nonceLen = Msg6NonceLen;

    hap_crypto_data_encrypt(crypto);
}

void HAPPairingsManager::onPairingDeviceEncryption(hap_pair_info * info, HAPUserHelper * request) {
    if(info->currentStep != 6) return;
    auto crypto = info->infoStore;
    uint8_t M6 = 6;

    auto response = tlv8_insert(
            nullptr,
            kTLVType_EncryptedData,
            crypto->dataLen + 16, //Encrypted data and authTag are allocated together
            crypto->encryptedData
    );
    response = tlv8_insert(response, kTLVType_State, 1, &M6);

    request->send(response);
    request->release();
}

hap_pair_info::~hap_pair_info() {
    delete setupStore;
}

hap_pair_info::hap_pair_info(HAPServer * server): server(server) { }

void hap_pair_info::renewInfoStore(HAPUserHelper * session) {
    delete infoStore;
    infoStore = new hap_crypto_info(server, session);
}
