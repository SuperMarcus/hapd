#include "common.h"

class HAPServer;
class HAPUserHelper;
class HAPPairingsManager;
class HAPPersistingStorage;
class BaseAccessory;
class BaseCharacteristic;
struct HAPEvent;
struct HAPEventListener;
struct tlv8_item;

#include "hap_request_helper.h"
#include "hap_events.h"
#include "hap_formats.h"
#include "hap_pair_info.h"

struct HAPSerializeOptions {
    bool withMeta = false;
    bool withPerms = false;
    bool withType = false;
    bool withEv = false;
};

class BaseCharacteristic {
private:
    friend class BaseAccessory;

    unsigned int characteristicIdentifier = 0;

    BaseCharacteristic * next = nullptr;
};

class BaseService {
private:
    friend class BaseCharacteristic;

    unsigned int serviceIdentifier = 0;

    BaseCharacteristic * characteristics = nullptr;
    BaseService * next = nullptr;
};

class BaseAccessory {
public:
    virtual BaseAccessory(unsigned int aid);

private:
    friend class HAPServer;

    /**
     * An unique id within this server
     */
    unsigned int accessoryIdentifier = 0;

    BaseService * services = nullptr;
    BaseAccessory * next = nullptr;
};

class HAPServer {
public:
    ~HAPServer();
    void begin(uint16_t port = 5001);

    /**
     * Must be called in every loop
     */
    void handle();

    /**
     * Add accessory to this server
     */
    void addAccessory(BaseAccessory *);

    //node-like event system but non-blocking so no wdt triggers :D
    HAPEventListener * on(HAPEvent::EventID, HAPEventListener::Callback);
    void emit(HAPEvent::EventID, void * args = nullptr, HAPEventListener::Callback onCompletion = nullptr);

    //The followings are used by network.c to encrypt/decrypt communications
    //between accessory and verified devices.
    void onInboundData(hap_network_connection *, uint8_t *body, uint8_t *tag, unsigned int bodyLen);
    void onOutboundData(hap_network_connection *, uint8_t *body, unsigned int bodyLen);

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
    void _onDeviceVerify(HAPEvent *);

    void _updateSDRecords(HAPEvent *);

    void _sendAttributionDatabase(HAPUserHelper *);

    hap_network_connection * server_conn = nullptr;
    HAPEvent * eventQueue = nullptr;
    HAPEventListener * eventListeners = nullptr;
    HAPPairingsManager * pairingsManager = nullptr;
    HAPPersistingStorage * storage = nullptr;
    BaseAccessory * accessories = nullptr;

private:
    friend class HAPPairingsManager;

    void * mdns_handle = nullptr;
    const char * deviceName = "HomeKit Device";
    const char * deviceId = "F6:A4:35:E3:0B:07";
    const char * modelName = "HomeKitDevice1,1";
    const char * setupCode = "816-32-958";
};

class HAPPairingsManager {
private:
    friend class HAPServer;
    explicit HAPPairingsManager(HAPServer *);

    void onPairSetup(HAPUserHelper *);
    void onPairSetupM2Finish(hap_crypto_setup *);
    void onPairSetupM4Finish(hap_crypto_setup *);

    void onPairVerify(HAPUserHelper *);

    void onPairingDeviceDecryption(hap_pair_info *, HAPUserHelper *);
    void onPairingDeviceEncryption(hap_pair_info *, HAPUserHelper *);
    void onVerifyingDeviceDecryption(hap_pair_info *, HAPUserHelper *);
    void onVerifyingDeviceEncryption(hap_pair_info *, HAPUserHelper *);
    void onDevicePaired(hap_pair_info *, HAPUserHelper *);
    void onDeviceVerified(hap_pair_info *, HAPUserHelper *);

    HAPServer * server;
};

extern HAPServer HKAccessory;
