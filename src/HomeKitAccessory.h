#include "common.h"

class HAPServer;
class HAPUserHelper;
class HAPPairingsManager;
class HAPPersistingStorage;
struct HAPEvent;
struct HAPEventListener;

struct tlv8_item;

class HAPUserHelper {
public:
    explicit HAPUserHelper(hap_network_connection * conn);
    ~HAPUserHelper();

    //simple arc
    void retain();
    void release();

    template <typename T = uint8_t *>
    T data(){ return static_cast<T>(conn->user->request_buffer); }
    unsigned int dataLength();
    hap_http_path path();
    hap_http_method method();
    hap_http_content_type requestContentType();
    hap_pair_info * pairInfo();

    void setResponseStatus(int status);
    void setResponseType(hap_msg_type type);
    void setContentType(hap_http_content_type responseCtype);
    void setBody(const void * body = nullptr, unsigned int contentLength = 0);
    void send(const char * body, int contentLength = -1);
    void send(const void * body = nullptr, unsigned int contentLength = 0);
    void send(tlv8_item * body);
    void send(int status, hap_msg_type type = MESSAGE_TYPE_UNKNOWN);

    void close();

private:
    hap_network_connection * conn;
    unsigned int refCount;
};

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

    template <typename T = void *>
    T* arg(){ return static_cast<T*>(argument); }

private:
    friend class HAPServer;

    EventID name = DUMMY;
    void * argument = nullptr;
    void (* didEmit)(HAPEvent * event) = nullptr;

    HAPEvent * next = nullptr;
};

struct HAPEventListener {
public:
    typedef void (* Callback)(HAPEvent *);
    typedef void (HAPServer::*HAPCallback)(HAPEvent *);

private:
    friend class HAPServer;

    HAPEvent::EventID listening = HAPEvent::DUMMY;
    Callback onEvent = nullptr;
    HAPCallback _internalOnEvent = nullptr;

    HAPEventListener * next = nullptr;
};

class HAPServer {
public:
    ~HAPServer();
    void begin(uint16_t port = 5001);
    void handle();
    void setPairingIdentifier(const char * uuid);

    //node-like event system but non-blocking so no wdt triggers :D
    HAPEventListener * on(HAPEvent::EventID, HAPEventListener::Callback);
    void emit(HAPEvent::EventID, void * args = nullptr, HAPEventListener::Callback onCompletion = nullptr);

private:
    HAPEventListener * _onSelf(HAPEvent::EventID, HAPEventListener::HAPCallback);
    void _clearEventQueue();
    void _clearEventListeners();
    HAPEvent * _dequeueEvent();

    void _onRequestReceived(HAPEvent *);
    void _onConnect(HAPEvent *);
    void _onDisconnect(HAPEvent *);
    void _onSetupInitComplete(HAPEvent *);
    void _onSetupProofComplete(HAPEvent *);
    void _onDataDecrypted(HAPEvent *);
    void _onDataEncrypted(HAPEvent *);
    void _onInitKeypairReq(HAPEvent *);
    void _onDevicePair(HAPEvent *);

    void _updateSDRecords(HAPEvent *);

    hap_network_connection * server_conn = nullptr;
    HAPEvent * eventQueue = nullptr;
    HAPEventListener * eventListeners = nullptr;
    HAPPairingsManager * pairingsManager = nullptr;
    HAPPersistingStorage * storage = nullptr;

private:
    friend class HAPPairingsManager;

    void * mdns_handle = nullptr;
    const char * deviceName = "HomeKit Device";
    const char * deviceId = "F6:A4:35:E3:0B:E2";
    const char * modelName = "HomeKitDevice1,1";
    const char * setupCode = "816-32-958";
};

struct hap_pair_info{
public:
    ~hap_pair_info();

private:
    friend class HAPServer;
    friend class HAPPairingsManager;

    bool isPaired = false;
    bool isVerifying = false;
    bool isPairing = false;
    uint8_t currentStep = 0;

    //Only used during setup
    hap_crypto_setup * setupStore = nullptr;
    hap_crypto_info * infoStore = nullptr;

    HAPServer * server;

    explicit hap_pair_info(HAPServer *);
    void renewInfoStore(HAPUserHelper *);
};

class HAPPairingsManager {
private:
    friend class HAPServer;
    explicit HAPPairingsManager(HAPServer *);

    void onPairSetup(HAPUserHelper *);
    void onPairSetupM2Finish(hap_crypto_setup *);
    void onPairSetupM4Finish(hap_crypto_setup *);

    void onPairingDeviceDecryption(hap_pair_info *, HAPUserHelper *);
    void onPairingDeviceEncryption(hap_pair_info *, HAPUserHelper *);
    void onDevicePaired(hap_pair_info *, HAPUserHelper *);

    HAPServer * server;
};

extern HAPServer HKAccessory;
