//
// Created by Xule Zhou on 8/6/18.
//

#ifndef ARDUINOHOMEKIT_HAP_EVENTS_H
#define ARDUINOHOMEKIT_HAP_EVENTS_H

struct HAPEvent {
public:
    enum EventID {
        /**
         * Nothing and shouldn't be triggered in production
         */
                DUMMY = 0,

        /**
         * Triggered when new connection is established
         */
                HAP_NET_CONNECT,

        /**
         * Triggered when new data from client is received.
         */
                HAP_NET_RECEIVE_REQUEST,

        /**
         * Triggered when a client is disconnected from the server
         * Called both when the client disconnects from the server and the
         * server disconnects the client.
         */
                HAP_NET_DISCONNECT,

        /**
         * Triggered when the parameters of the server have updated and the
         * mDNS service discovery needs to be updated as well.
         *
         * Handled internally by HAPServer
         */
                HAP_SD_NEEDED_UPDATE,

        /**
         * Tells HAPServer to generate new AccessoryLTPK and AccessoryLTSK,
         * and persistently stores them. This is very dangerous to use.
         */
                HAP_INITIALIZE_KEYPAIR,

        /**
         * Called when a new device is being paired with this accessory.
         */
                HAP_DEVICE_PAIR,

        /**
         * Called when a paired device is successfully verified
         */
                HAP_DEVICE_VERIFY,

        /**
         * The followings are cryptography yields. Nothing besides
         * HAPPairingsManager and crypto impl should listen to
         * those events.
         */

                HAPCRYPTO_SRP_INIT_FINISH_GEN_SALT,
        HAPCRYPTO_SRP_INIT_COMPLETE, //Handled by HAPServer

        HAPCRYPTO_SRP_PROOF_VERIFIER_CREATED,
        HAPCRYPTO_SRP_PROOF_SKEY_GENERATED,
        HAPCRYPTO_SRP_PROOF_SSIDE_GENERATED,
        HAPCRYPTO_SRP_PROOF_COMPLETE, //Handled by HAPServer

        //ChachaPoly encrypt/decrypt
                HAPCRYPTO_NEED_ENCRYPT,
        HAPCRYPTO_NEED_DECRYPT,
        HAPCRYPTO_ENCRYPTED, //Handled by HAPServer
        HAPCRYPTO_DECRYPTED, //Handled by HAPServer

#ifdef USE_ASYNC_MATH
        HAPCRYPTO_ASYNC_EXPMOD_BODY,
        HAPCRYPTO_ASYNC_EXPMOD_FINAL
#endif
    };

    template<typename T = void *>
    T *arg() { return static_cast<T *>(argument); }

private:
    friend class HAPServer;

    EventID name = DUMMY;
    void *argument = nullptr;

    void (*didEmit)(HAPEvent *event) = nullptr;

    HAPEvent *next = nullptr;
};

struct HAPEventListener {
public:
    typedef void (*Callback)(HAPEvent *);

    typedef void (HAPServer::*HAPCallback)(HAPEvent *);

private:
    friend class HAPServer;

    HAPEvent::EventID listening = HAPEvent::DUMMY;
    Callback onEvent = nullptr;
    HAPCallback _internalOnEvent = nullptr;

    HAPEventListener *next = nullptr;
};

struct CharacteristicSubscriber {
private:
    friend class HAPServer;

    HAPUserHelper * session = nullptr;
    BaseCharacteristic * listening = nullptr;

    CharacteristicSubscriber *next = nullptr;
};

#endif //ARDUINOHOMEKIT_HAP_EVENTS_H
