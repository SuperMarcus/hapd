#include <utility>
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

unsigned int _nextIid(HAPServer *);

struct HAPSerializeOptions {
    bool withMeta = false;
    bool withPerms = false;
    bool withType = false;
    bool withEv = false;
    unsigned int aid = 0;
};

class BaseCharacteristic {
public:
    SCONST uint32_t type = 0x00000000;

    HAPUserHelper * lastOperator = nullptr;

protected:
    friend class BaseAccessory;
    friend class BaseService;
    friend class HAPServer;

    explicit BaseCharacteristic(unsigned int parentAccessory, HAPServer * server, uint32_t type);

    void setValue(CharacteristicValue v, HAPUserHelper * sender = nullptr);

    unsigned int instanceIdentifier = 0;
    unsigned int accessoryIdentifier = 0;
    uint32_t characteristicTypeIdentifier = 0;

    CharacteristicValue value;
    CharacteristicValueFormat format;
    CharacteristicPermissions permissions;
    HAPServer * server = nullptr;
    BaseCharacteristic * next = nullptr;
};

class BaseService {
public:
    /**
     * Every service has a generic static type property, which
     * can be used to identify the service if we are sure only
     * a single service of this type is present in the
     * accessory.
     */
    SCONST uint32_t type = 0x00000000;

protected:
    friend class BaseCharacteristic;
    friend class BaseAccessory;
    friend class HAPServer;

    explicit BaseService(unsigned int parentAccessory, uint32_t type, HAPServer *);

    void addCharacteristic(BaseCharacteristic *);

    unsigned int accessoryIdentifier = 0;
    unsigned int instanceIdentifier = 0;
    uint32_t serviceTypeIdentifier = 0;
    uint8_t flags = 0;

    BaseCharacteristic * characteristics = nullptr;
    HAPServer * server = nullptr;
    BaseService * next = nullptr;
};

class BaseAccessory {
public:
    template <typename T>
    T * getService(){
        BaseService * find = nullptr;
        auto current = services;
        while (current != nullptr) {
            if(current->serviceTypeIdentifier == T::type)
                find = current;
            current = current->next;
        }
        return find;
    }

    template <typename T, typename ...Args>
    T * addService(Args&&... args){
        auto s = new T(accessoryIdentifier, server, std::forward<Args>(args)...);
        _addService(s);
        return s;
    }

    /**
     * Get the characteristic on this accessory with the given
     * instance identifier.
     *
     * @param iid
     * @return
     */
    BaseCharacteristic * getCharacteristic(unsigned int iid);

private:
    friend class HAPServer;

    explicit BaseAccessory(unsigned int aid, HAPServer *);

    void _addService(BaseService *);

    /**
     * An unique id within this server
     */
    unsigned int accessoryIdentifier = 1;

    BaseService * services = nullptr;
    HAPServer * server = nullptr;
    BaseAccessory * next = nullptr;
};

class HAPServer {
public:
    ~HAPServer();

    /**
     * Start the HAPServer, listen to the provided port, and
     * broadcast this server via mDNS.
     *
     * @note All the accessories should be added to this server
     * before calling this function. Also, please make sure to
     * call HAPServer::handle() in every loop to process events.
     *
     * @param port The port that the server will be listening on
     */
    void begin(uint16_t port = 5001);

    /**
     * Must be called in every loop
     */
    void handle();

    /**
     * Get the accessory with aid. Pass 1 to obtain the main
     * accessory for this server.
     *
     * @param aid Accessory identifier
     */
    BaseAccessory * getAccessory(unsigned int aid = 1);

    /**
     * Get the instance of Characteristic with its instance
     * identifier (iid)
     *
     * @param iid Instance identifier of the characteristic
     * @param aid Accessory identifier, default to 1, which is the main accessory
     * @return
     */
    BaseCharacteristic * getCharacteristic(unsigned int iid, unsigned int aid = 1);

    /**
     * Check if the user is a subscriber of the characteristic
     *
     * @return true if the user is
     */
    bool isSubscriber(HAPUserHelper *, BaseCharacteristic *);

    /**
     * Make the user listen to changes in the provided characteristic
     */
    void subscribe(HAPUserHelper *, BaseCharacteristic *);

    /**
     * Remove the user as a subscriber to the provided characteristic
     */
    void unsubscribe(HAPUserHelper *, BaseCharacteristic *);

    //TODO:
//    BaseAccessory * addAccessory();

    //node-like event system but non-blocking so no wdt triggers :D
    HAPEventListener * on(HAPEvent::EventID, HAPEventListener::Callback);
    void emit(HAPEvent::EventID, void * args = nullptr, HAPEventListener::Callback onCompletion = nullptr);

    const char * modelName = "HomeKitDevice1,1";
    const char * deviceName = "HomeKit Device";

public:
    //The followings are used by network.c to encrypt/decrypt communications
    //between accessory and verified devices.
    void onInboundData(hap_network_connection *, uint8_t *body, uint8_t *tag, unsigned int bodyLen);
    void onOutboundData(hap_network_connection *, uint8_t *body, unsigned int bodyLen);
    void preDeviceDisconnection(hap_network_connection *);

private:
    friend class BaseAccessory;
    friend class BaseService;
    friend class BaseCharacteristic;

    /**
     * Add accessory to this server
     */
    void addAccessory(BaseAccessory *);

    unsigned int instanceIdPool = 1;

private:
    HAPEventListener * _onSelf(HAPEvent::EventID, HAPEventListener::HAPCallback);
    void _clearEventQueue();
    void _clearEventListeners();
    void _clearSubscribers();
    HAPEvent * _dequeueEvent();

    void _onRequestReceived(HAPEvent *);
    void _onSetupInitComplete(HAPEvent *);
    void _onSetupProofComplete(HAPEvent *);
    void _onDataDecrypted(HAPEvent *);
    void _onDataEncrypted(HAPEvent *);
    void _onInitKeypairReq(HAPEvent *);
    void _onDevicePair(HAPEvent *);
    void _onDeviceVerify(HAPEvent *);
    void _onCharUpdate(HAPEvent *);

    void _updateSDRecords();

    void _handleCharacteristicWrite(HAPUserHelper *);
    void _sendAttributionDatabase(HAPUserHelper *);
    unsigned int _serializeService(char *, unsigned int len, BaseService *, HAPUserHelper *, HAPSerializeOptions *);
    unsigned int _serializeCharacteristic(char *, unsigned int len, BaseCharacteristic *, HAPUserHelper *, HAPSerializeOptions *);
    unsigned int _serializeMultiCharacteristics(char *, unsigned int len, const char * aidIids, HAPUserHelper *, HAPSerializeOptions *);
    unsigned int _serializeUpdatedCharacteristics(char *, unsigned int len, BaseCharacteristic **, HAPUserHelper *, HAPSerializeOptions * options);

    hap_network_connection * server_conn = nullptr;
    HAPEvent * eventQueue = nullptr;
    HAPEventListener * eventListeners = nullptr;
    HAPPairingsManager * pairingsManager = nullptr;
    HAPPersistingStorage * storage = nullptr;
    BaseAccessory * accessories = nullptr;
    CharacteristicSubscriber * subscribers = nullptr;

private:
    friend class HAPPairingsManager;

    void * mdns_handle = nullptr;
    const char * deviceId = "F6:A4:35:E3:0B:09";
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
    void onPairingOperations(HAPUserHelper *);

    void onPairingDeviceDecryption(hap_pair_info *, HAPUserHelper *);
    void onPairingDeviceEncryption(hap_pair_info *, HAPUserHelper *);
    void onVerifyingDeviceDecryption(hap_pair_info *, HAPUserHelper *);
    void onVerifyingDeviceEncryption(hap_pair_info *, HAPUserHelper *);
    void onDevicePaired(hap_pair_info *, HAPUserHelper *);
    void onDeviceVerified(hap_pair_info *, HAPUserHelper *);

    HAPServer * server;
};

extern HAPServer HKAccessory;
