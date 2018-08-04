#include "HomeKitAccessory.h"
#include "HAPPersistingStorage.h"
#include "network.h"
#include "tlv.h"
#include "hap_crypto.h"

#include <cstring>

#define IOS_DEVICE_INFO_LEN     100
#define ACCESSORY_INFO_LEN      81
#define DEVICE_ID_LEN           17
#define IOS_PAIRING_ID_LEN      36

HAPPairingsManager::HAPPairingsManager(HAPServer * server): server(server) {
    //Add crypto callbacks to server
    hap_crypto_init(server);

    //If no accessory keypair, then init one
    if(!server->storage->haveAccessoryLongTermKeys()){
        server->emit(HAPEvent::HAP_INITIALIZE_KEYPAIR);
    }
}

void HAPPairingsManager::onPairSetup(HAPUserHelper * request) {
    if(request->requestContentType() != HAP_PAIRING_TLV8){
        request->sendError(kTLVError_Unknown);
        return;
    }

    auto pairInfo = request->pairInfo();
    auto& setupStore = pairInfo->setupStore;
    auto chain = tlv8_parse(request->data(), request->dataLength());
    auto requestPairStep = *tlv8_find(chain, kTLVType_State)->value;
    HAP_DEBUG("pairing request: state(%d)", requestPairStep);

    if(pairInfo->isPaired){
        HAP_DEBUG("Weird...client is already paired, but it requests to pair again.");
        request->sendError(kTLVError_Unknown);
        return;
    }

    if(pairInfo->currentStep + 1 != requestPairStep || (!pairInfo->isPairing && requestPairStep > 1)){
        HAP_DEBUG("HAP Client tries to skip step. Warned.");
        request->sendError(kTLVError_Authentication);
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
        auto deviceId = new uint8_t[36];
        info->setupStore->deviceLtpk = deviceLtpk;
        info->setupStore->deviceId = deviceId;

        tlv8_read(tlv8_find(chain, kTLVType_PublicKey), deviceLtpk, 32);
        tlv8_read(tlv8_find(chain, kTLVType_Signature), signature, 64);
        tlv8_read(tlv8_find(chain, kTLVType_Identifier), deviceId, 36);

        //iOSDeviceInfo = iOSDeviceX, iOSDevicePairingID, iOSDeviceLTPK
        auto iOSDeviceInfo = new uint8_t[IOS_DEVICE_INFO_LEN];
        hap_crypto_derive_key(iOSDeviceInfo, info->setupStore->sessionKey, hkdfSalt, hkdfInfo);
        memcpy(iOSDeviceInfo + 32, deviceId, 36);
        memcpy(iOSDeviceInfo + 32 + 36, deviceLtpk, 32);

        tlv8_free(chain);

        if(hap_crypto_longterm_verify(signature, iOSDeviceInfo, IOS_DEVICE_INFO_LEN, deviceLtpk)){
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

    request->sendError(kTLVError_Authentication);
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
    auto secKey = new uint8_t[64];
    server->storage->getAccessoryLongTermKeys(pubKey, secKey);

    //AccessoryInfo = AccessoryX, AccessoryPairingID, AccessoryLTPK
    auto AccessoryInfo = new uint8_t[ACCESSORY_INFO_LEN];
    hap_crypto_derive_key(AccessoryInfo, info->setupStore->sessionKey, hkdfSalt, hkdfInfo);
    memcpy(AccessoryInfo + 32, server->deviceId, DEVICE_ID_LEN);
    memcpy(AccessoryInfo + 32 + DEVICE_ID_LEN, pubKey, 32);

    auto signature = hap_crypto_sign(AccessoryInfo, ACCESSORY_INFO_LEN, pubKey, secKey);

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
    auto setupData = info->setupStore;
    uint8_t M6 = 6;

    auto response = tlv8_insert(
            nullptr,
            kTLVType_EncryptedData,
            crypto->dataLen + 16, //Encrypted data and authTag are allocated together
            crypto->encryptedData
    );
    response = tlv8_insert(response, kTLVType_State, 1, &M6);

    //Persistently stores the device as a paired device
    server->storage->addPairedDevice(setupData->deviceId, setupData->deviceLtpk);

    request->send(response);
    request->release();
}

void HAPPairingsManager::onPairVerify(HAPUserHelper * request) {
    if(request->requestContentType() != HAP_PAIRING_TLV8){
        request->sendError(kTLVError_Unknown);
        return;
    }

    auto info = request->pairInfo();
    auto& crypto = info->infoStore;
    auto& store = info->verifyStore;

    auto chain = tlv8_parse(request->data(), request->dataLength());
    auto step = *tlv8_find(chain, kTLVType_State)->value;
    HAP_DEBUG("verify request: state(%d)", step);

    if(info->isVerifying && (step - 1) != info->currentStep){
        HAP_DEBUG("HAP Client tries to skip step. Warned.");
        request->sendError(kTLVError_Authentication);
        return;
    }

    if(step == 1){
        info->isVerifying = true;
        info->renewInfoStore(request);
        delete store;
        store = new hap_crypto_verify();
    }

    if(!info->isVerifying){
        request->sendError(kTLVError_Unknown);
        return;
    }

    crypto->session = request;
    switch (step){
        case 1: { //M1
            const constexpr static uint8_t Msg2Nonce[] = "PV-Msg02";
            const constexpr static unsigned int Msg2NonceLen = sizeof(Msg2Nonce) - 1;
            const constexpr static char hkdfSalt[] = "Pair-Verify-Encrypt-Salt";
            const constexpr static char hkdfInfo[] = "Pair-Verify-Encrypt-Info";

            uint8_t AccessoryInfo[ACCESSORY_INFO_LEN];
            uint8_t AccessoryLTPK[32];
            uint8_t AccessoryLTSK[64];

            server->storage->getAccessoryLongTermKeys(AccessoryLTPK, AccessoryLTSK);

            //Generate new curve25519 keypair
            hap_crypto_ephemeral_keypair(store->ePubKey, store->eSecKey);

            //Read iOS device's Curve25519 public key
            tlv8_read(tlv8_find(chain, kTLVType_PublicKey), store->iOSePubKey, 32);

            //Generate shared secret
            hap_crypto_ephemeral_exchange(store);

            //Derive encryption keys
            hap_crypto_derive_key(crypto->key, store->eSharedSecret, hkdfSalt, hkdfInfo, 32);

            memcpy(AccessoryInfo, store->ePubKey, 32);
            memcpy(AccessoryInfo + 32, server->deviceId, DEVICE_ID_LEN);
            memcpy(AccessoryInfo + 32 + DEVICE_ID_LEN, store->iOSePubKey, 32);

            auto signature = hap_crypto_sign(AccessoryInfo, ACCESSORY_INFO_LEN, AccessoryLTPK, AccessoryLTSK);
            auto subtlv = tlv8_insert(nullptr, kTLVType_Signature, 64, signature);
            tlv8_insert(subtlv, kTLVType_Identifier, DEVICE_ID_LEN, server->deviceId);

            unsigned int subtlvLen = 0;
            auto exportedSubtlv = tlv8_export_free(subtlv, &subtlvLen);
            delete[] signature;

            crypto->reset();
            crypto->dataLen = subtlvLen;
            crypto->rawData = exportedSubtlv;
            crypto->nonce = Msg2Nonce;
            crypto->nonceLen = Msg2NonceLen;

            //Release in HAPPairingsManager::onVerifyingDeviceEncryption
            request->retain();
            hap_crypto_data_encrypt(crypto);
            info->currentStep = 2;

            break;
        }
        case 3: { //M3
            auto subtlv = tlv8_find(chain, kTLVType_EncryptedData);
            const constexpr static uint8_t Msg3Nonce[] = "PV-Msg03";
            const constexpr static unsigned int Msg3NonceLen = sizeof(Msg3Nonce) - 1;

            crypto->reset();
            crypto->dataLen = tlv8_value_length(subtlv);
            crypto->encryptedData = new uint8_t[crypto->dataLen];
            tlv8_read(subtlv, crypto->encryptedData, crypto->dataLen);
            crypto->dataLen -= 16;
            crypto->authTag = crypto->encryptedData + crypto->dataLen;
            crypto->nonce = Msg3Nonce;
            crypto->nonceLen = Msg3NonceLen;

            request->retain();
            hap_crypto_data_decrypt(crypto);
            info->currentStep = 4;

            break;
        }
        default: HAP_DEBUG("Unimplemented verify step %d", step);
    }

    tlv8_free(chain);
}

void HAPPairingsManager::onVerifyingDeviceEncryption(hap_pair_info * info, HAPUserHelper * request) {
    if(info->currentStep != 2) return;

    auto crypto = info->infoStore;
    auto store = info->verifyStore;
    uint8_t M2 = 2;

    auto response = tlv8_insert(nullptr, kTLVType_EncryptedData, crypto->dataLen + 16, crypto->encryptedData);
    response = tlv8_insert(response, kTLVType_PublicKey, 32, store->ePubKey);
    response = tlv8_insert(response, kTLVType_State, 1, &M2);

    request->send(response);
    request->release();
}

void HAPPairingsManager::onVerifyingDeviceDecryption(hap_pair_info * info, HAPUserHelper * request) {
    if(info->currentStep != 4) return;

    auto crypto = info->infoStore;
    auto store = info->verifyStore;

    if(hap_crypto_data_decrypt_did_succeed(crypto)){
        auto subtlv = tlv8_parse(crypto->rawData, crypto->dataLen);

        auto ident = new uint8_t[IOS_PAIRING_ID_LEN];
        auto signature = new uint8_t[64];
        tlv8_read(tlv8_find(subtlv, kTLVType_Identifier), ident, IOS_PAIRING_ID_LEN);
        tlv8_read(tlv8_find(subtlv, kTLVType_Signature), signature, 64);

        auto pairedDevice = server->storage->retrievePairedDevice(ident);
        delete[] ident;

        if(pairedDevice){
            uint8_t iOSDeviceInfo[IOS_DEVICE_INFO_LEN];
            memcpy(iOSDeviceInfo, store->iOSePubKey, 32);
            memcpy(iOSDeviceInfo + 32, ident, IOS_PAIRING_ID_LEN);
            memcpy(iOSDeviceInfo + 32 + IOS_PAIRING_ID_LEN, store->ePubKey, 32);

            if(hap_crypto_longterm_verify(signature, iOSDeviceInfo, IOS_DEVICE_INFO_LEN, pairedDevice->publicKey)){
                delete[] signature;
                delete pairedDevice;
                tlv8_free(subtlv);
                server->emit(HAPEvent::HAP_DEVICE_VERIFY, info);
                return;
            }
        }

        delete[] signature;
        tlv8_free(subtlv);
    }

    request->sendError(kTLVError_Authentication);
    request->release();
}

void HAPPairingsManager::onDeviceVerified(hap_pair_info * info, HAPUserHelper * request) {
    const constexpr static char ctlSalt[] = "Control-Salt";
    const constexpr static char ctlReadInfo[] = "Control-Read-Encryption-Key";
    const constexpr static char ctlWriteInfo[] = "Control-Write-Encryption-Key";

    uint8_t M4 = 4;
    auto& store = info->verifyStore;

    auto response = tlv8_insert(nullptr, kTLVType_State, 1, &M4);
    request->send(response);
    request->release();

    info->isVerifying = false;
    info->isPaired = true;

    //Derive two control keys
    hap_crypto_derive_key(info->AccessoryToControllerKey, store->eSharedSecret, ctlSalt, ctlReadInfo, 32);
    hap_crypto_derive_key(info->ControllerToAccessoryKey, store->eSharedSecret, ctlSalt, ctlWriteInfo, 32);

    delete store;
    store = nullptr;
}

hap_pair_info::~hap_pair_info() {
    delete setupStore;
    delete infoStore;
    delete verifyStore;
}

hap_pair_info::hap_pair_info(HAPServer * server): server(server) { }

void hap_pair_info::renewInfoStore(HAPUserHelper * session) {
    delete infoStore;
    infoStore = new hap_crypto_info(server, session);
}

bool hap_pair_info::paired() {
    return isPaired;
}

hap_crypto_info *hap_pair_info::prepare(bool isWrite, hap_network_connection * conn) {
    auto crypto = infoStore;
    crypto->reset();
    memcpy(crypto->key, isWrite ? AccessoryToControllerKey : ControllerToAccessoryKey, 32);
    memset(nonceStore, 0, 8);
    auto cnt = isWrite ? writeCount : readCount;
    for(auto& n : nonceStore){
        n = static_cast<uint8_t>(cnt % 0xff);
        cnt /= 0xff;
    }
    (*(isWrite ? &writeCount : &readCount))++;
    crypto->nonce = nonceStore;
    crypto->nonceLen = 8;
    crypto->conn = conn;
    crypto->flags |= CRYPTO_FLAG_NETWORK;
    return crypto;
}
