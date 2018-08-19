/**
 * hapd
 *
 * Copyright 2018 Xule Zhou
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef HAPD_HAP_REQUEST_HELPER_H
#define HAPD_HAP_REQUEST_HELPER_H

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
    const hap_http_request_parameters * params();

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
    friend class HAPServer;
    hap_network_connection * conn;
    unsigned int refCount;
};

#endif //HAPD_HAP_REQUEST_HELPER_H
