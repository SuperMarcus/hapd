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

#ifndef HAPD_HAP_PAIR_INFO_H
#define HAPD_HAP_PAIR_INFO_H

struct hap_pair_info{
public:
    explicit hap_pair_info(HAPServer *);
    ~hap_pair_info();
    bool paired();
    hap_crypto_info * prepare(bool isWrite, hap_network_connection *);

private:
    friend class HAPServer;
    friend class HAPPairingsManager;
    friend class HAPUserHelper;

    bool isPaired = false;
    bool isVerifying = false;
    bool isPairing = false;
    uint8_t currentStep = 0;

    //Only used during setup/verify
    hap_crypto_setup * setupStore = nullptr;
    hap_crypto_info * infoStore = nullptr;
    hap_crypto_verify * verifyStore = nullptr;

    uint8_t identifier[36];
    uint8_t AccessoryToControllerKey[32];
    uint8_t ControllerToAccessoryKey[32];
    uint8_t nonceStore[8];
    uint32_t writeCount = 0;
    uint32_t readCount = 0;

    HAPServer * server;

    void renewInfoStore(HAPUserHelper *);
};

#endif //HAPD_HAP_PAIR_INFO_H
