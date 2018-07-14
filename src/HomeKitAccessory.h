#include "network.h"

class HAPServer;
class HAPUserHelper;
class HAPPairingsManager;
struct HAPEvent;
struct HAPEventListener;

class HAPUserHelper {
public:
    explicit HAPUserHelper(hap_network_connection * conn);
    ~HAPUserHelper();

    void setResponseStatus(int status);
    void setResponseType(hap_msg_type type);
    void setContentType(hap_http_content_type responseCtype);
    void setBody(const void * body = nullptr, unsigned int contentLength = 0);
    void send(const char * body, int contentLength = -1);
    void send(const void * body = nullptr, unsigned int contentLength = 0);
    void send(int status, hap_msg_type type = HTTP_1_1);

private:
    hap_network_connection * conn;
};

struct HAPEvent {
public:
    enum EventID {
        /**
         * Nothing and shouldn't be triggered in production
         */
        DUMMY = 0,

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
        HAP_SD_NEEDED_UPDATE
    };
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

    //node-like event system but non-blocking so no wdt triggers :D
    HAPEventListener * on(HAPEvent::EventID, HAPEventListener::Callback);
    void emit(HAPEvent::EventID, void * args = nullptr, HAPEventListener::Callback onCompletion = nullptr);

private:
    HAPEventListener * _onSelf(HAPEvent::EventID, HAPEventListener::HAPCallback);
    void _onRequestReceived(HAPEvent * conn);
    void _clearEventQueue();
    void _clearEventListeners();
    HAPEvent * _dequeueEvent();

    void _updateSDRecords(HAPEvent *);

    hap_network_connection * server_conn = nullptr;
    HAPEvent * eventQueue = nullptr;
    HAPEventListener * eventListeners = nullptr;
    HAPPairingsManager * pairingsManager = nullptr;

    void * mdns_handle = nullptr;
    const char * deviceName = "HomeKit Device";
    const char * deviceId = "F6:A4:35:E3:0A:E2";
    const char * modelName = "HomeKitDevice1,1";
};

class HAPPairingsManager {

};

extern HAPServer HKAccessory;
