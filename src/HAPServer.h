#include "network.h"
#include "tlv.h"

class HAPUserHelper {
public:
    explicit HAPUserHelper(hap_network_connection * conn);
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
    enum EventID {
        DUMMY
    };

    EventID name = DUMMY;
    void * argument = nullptr;
    void (* didEmit)(HAPEvent * event) = nullptr;

    HAPEvent * next = nullptr;
};

struct HAPEventListener {
    typedef void (* Callback)(HAPEvent *);

    HAPEvent::EventID listening = HAPEvent::DUMMY;
    Callback onEvent = nullptr;

    HAPEventListener * next = nullptr;

    void operator ()(HAPEvent *);
};

class HAPServer {
public:
    void begin(uint16_t port = 5001);
    void handle();

    //node-like event system but non-blocking so no wdt triggers :D
    void on(HAPEvent::EventID, HAPEventListener::Callback);
    void emit(HAPEvent::EventID, void * args = nullptr, HAPEventListener::Callback onCompletion);

private:
    friend void hap_event_network_receive(hap_network_connection *, const uint8_t *, unsigned int);
    static void _s_onRequestReceived(hap_network_connection * conn, void * arg);

    void _onRequestReceived(hap_network_connection * conn);
    void _clearEventQueue();
    HAPEvent * _dequeueEvent();

    hap_network_connection * server_conn = nullptr;
    HAPEvent * eventQueue = nullptr;
    HAPEventListener * eventListeners = nullptr;
};

extern HAPServer HomeKitAccessory;
