//
// Created by Xule Zhou on 8/6/18.
//

#ifndef ARDUINOHOMEKIT_HAP_REQUEST_HELPER_H
#define ARDUINOHOMEKIT_HAP_REQUEST_HELPER_H

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
    void send();
    void send(const char * body);
    void send(const void * body, unsigned int contentLength);
    void send(tlv8_item * body);
    void send(int status, hap_msg_type type = MESSAGE_TYPE_UNKNOWN);
    void sendError(uint8_t, int status = HTTP_400_BAD_REQUEST);

    void close();

    inline bool equals(HAPUserHelper * another) const
    { return conn == another->conn; }

    inline bool equals(hap_network_connection * conn) const
    { return conn == this->conn; }

private:
    hap_network_connection * conn;
    unsigned int refCount;
};

#endif //ARDUINOHOMEKIT_HAP_REQUEST_HELPER_H
