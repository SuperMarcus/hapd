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

#include "HomeKitAccessory.h"
#include "json/jsmn.h"

SCONST char _serviceBeginFmt[] PROGMEM = R"({"type":"%X","iid":%d,"characteristics":[)";
SCONST char _bEnd[] PROGMEM = R"(]})";
SCONST char _charMultiBegin[] PROGMEM = R"({"characteristics":[)";

SCONST char _typeFmt[] PROGMEM = R"("type":"%X")";
SCONST char _iidFmt[] PROGMEM = R"("iid":%d)";
SCONST char _evFmt[] PROGMEM = R"("ev":%s)";
SCONST char _formatFmt[] PROGMEM = R"("format":"%s")";
SCONST char _aidFmt[] PROGMEM = R"("aid":%u)";
SCONST char _permsFmt[] PROGMEM = R"("perms":[)";

SCONST char _valueStrFmt[] PROGMEM = R"("value":"%s")";
SCONST char _valueFloatFmt[] PROGMEM = R"("value":%Lf)";
SCONST char _valueIntFmt[] PROGMEM = R"("value":%d)";
SCONST char _valueUIntFmt[] PROGMEM = R"("value":%u)";
SCONST char _valueRawStrFmt[] PROGMEM = R"("value":%s)";

SCONST char _bool_true[] = "true";
SCONST char _bool_false[] = "false";

#ifndef PGM_R
#define PGM_R(t, ptr) \
    auto (t) = new char[strlen_P(ptr) + 1](); \
    strcpy_P(t, ptr);
#endif

#define bptr (buf + tot)
#define bpush(c) do{ \
        if(bufLen > 0) *bptr = c; \
        tot += 1; \
    }while(0)
#define bpop() do{ \
        tot -= 1; \
        if(bufLen > 0) *bptr = '\x00';\
    }while(0)
#define bcpy(ptr, len) do{ \
        if(bufLen > 0) memcpy(bptr, ptr, len); \
        tot += (len); \
    }while(0)
#define bprintf(fmt, ...) \
    tot += snprintf(bptr, bufLen, fmt, ##__VA_ARGS__)
#define bstr(str) do{ \
        const char _s[] = str; \
        bcpy(_s, sizeof(_s) - 1); \
    }while(0)
#define bpprintf(fmt_P, ...) do{ \
        PGM_R(fmt, fmt_P); \
        bprintf(fmt, ##__VA_ARGS__); \
        delete[] fmt; \
    }while(0)
#define bpstr(str_P) do{ \
        auto _len = strlen_P(str_P); \
        if(bufLen > 0) memcpy_P(bptr, str_P, _len); \
        tot += _len; \
    }while(0)

//Remember to delete[] after use
static const char *getValueFormatFmt(CharacteristicValueFormat f) {
    const char *ptr;
    switch (f) {
        case FORMAT_UINT8:
        case FORMAT_UINT16:
        case FORMAT_UINT32:
        case FORMAT_UINT64:
            ptr = _valueUIntFmt;
            break;
        case FORMAT_INT:
            ptr = _valueIntFmt;
            break;
        case FORMAT_BOOL:
            ptr = _valueRawStrFmt;
            break;
        case FORMAT_FLOAT:
            ptr = _valueFloatFmt;
            break;
        case FORMAT_TLV8:
        case FORMAT_DATA:
        case FORMAT_STRING:
        default:
            ptr = _valueStrFmt;
    }
    PGM_R(buf, ptr);
    return buf;
}

static const char *getFormatStr(CharacteristicValueFormat f) {
    switch (f) {
        case FORMAT_BOOL:
            return "bool";
        case FORMAT_UINT8:
            return "uint8";
        case FORMAT_UINT16:
            return "uint16";
        case FORMAT_UINT32:
            return "uint32";
        case FORMAT_UINT64:
            return "uint64";
        case FORMAT_INT:
            return "int";
        case FORMAT_FLOAT:
            return "float";
        case FORMAT_TLV8:
            return "tlv8";
        case FORMAT_DATA:
            return "data";

            //Default to string
        case FORMAT_STRING:
        default:
            return "string";
    }
}

static const CharacteristicValue _v(CharacteristicValue v, CharacteristicValueFormat f) {
    switch (f) {
        case FORMAT_BOOL:
            return CharacteristicValue{
                    .string_value = v.bool_value ? _bool_true : _bool_false
            };
        default:
            return v;
    }
}

unsigned int HAPServer::_serializeCharacteristic(
        char *buf,
        unsigned int bufLen,
        BaseCharacteristic * c,
        HAPUserHelper * session,
        HAPSerializeOptions * options
) {
    unsigned int tot = 0;

    bpush('{');

    if (options->withMeta) {
        bpprintf(_formatFmt, getFormatStr(c->format));
        bpush(',');
    }

    if (options->withType) {
        bpprintf(_typeFmt, c->characteristicTypeIdentifier);
        bpush(',');
    }

    if (options->withPerms) {
        // "perms:["
        bpstr(_permsFmt);
        if((c->permissions) & CPERM_PW) bstr(R"("pw",)"); // NOLINT
        if((c->permissions) & CPERM_PR) bstr(R"("pr",)"); // NOLINT
        if((c->permissions) & CPERM_EV) bstr(R"("ev",)"); // NOLINT
        if((c->permissions) & CPERM_AA) bstr(R"("aa",)"); // NOLINT
        if((c->permissions) & CPERM_TW) bstr(R"("tw",)"); // NOLINT
        if((c->permissions) & CPERM_HD) bstr(R"("hd",)"); // NOLINT
        //Pop one out, there must be at least one permissions
        bpop();
        // ]
        bpush(']');
        bpush(',');
    }

    if (options->withEv) {
        bpprintf(_evFmt, isSubscriber(session, c) ? _bool_true : _bool_false);
        bpush(',');
    }

    if (options->aid > 0) {
        bpprintf(_aidFmt, options->aid);
        bpush(',');
    }

    //Only send value when the characteristic have PairedRead permission
    if((c->permissions) & CPERM_PR){ // NOLINT
        auto valFmt = getValueFormatFmt(c->format);
        bprintf(valFmt, _v(c->value, c->format));
        delete[] valFmt;
        bpush(',');
    }

    bpprintf(_iidFmt, c->instanceIdentifier);

    bpush('}');

    return tot;
}

unsigned int HAPServer::_serializeService(
        char *buf,
        unsigned int bufLen,
        BaseService *service,
        HAPUserHelper *session,
        HAPSerializeOptions *options
) {
    auto c = service->characteristics;
    unsigned int tot = 0;

    PGM_R(serviceBeginFormat, _serviceBeginFmt);
    tot += snprintf(bptr, bufLen, serviceBeginFormat, service->serviceTypeIdentifier, service->instanceIdentifier);
    delete[] serviceBeginFormat;

    while (c != nullptr) {
        tot += _serializeCharacteristic(bptr, bufLen, c, session, options);
        c = c->next;
        if (c != nullptr) { bpush(','); }
    }

    PGM_R(serviceEnd, _bEnd);
    bprintf("%s", serviceEnd);
    delete[] serviceEnd;

    return tot;
}

void HAPServer::_sendAttributionDatabase(HAPUserHelper *request) {
    SCONST char _beginAcc[] = R"({"accessories":[)";
    SCONST char _beginAccSrvFmt[] = R"({"aid":%u,"services":[)";
    SCONST char _end[] = "]}";
    SCONST unsigned int _beginLen = sizeof(_beginAcc) - 1;
    SCONST unsigned int _endLen = sizeof(_end) - 1;
    SCONST unsigned int _wrpLen = _beginLen + _endLen;

    HAPSerializeOptions options;
    options.withEv = false;
    options.withMeta = true;
    options.withPerms = true;
    options.withType = true;

    unsigned int bufLen = _wrpLen;
    auto current = accessories;

    //Calculate the size of memory needed
    while (current != nullptr) {
        bufLen += snprintf(nullptr, 0, _beginAccSrvFmt, current->accessoryIdentifier);

        auto s = current->services;
        while (s != nullptr) {
            bufLen += _serializeService(nullptr, 0, s, request, &options);
            s = s->next;
            if (s != nullptr) bufLen += 1;
        }

        bufLen += _endLen;

        current = current->next;
        bufLen += current == nullptr ? 0 : 1;
    }

    auto buf = new char[bufLen + 1]();
    auto tot = 0;

    // {"accessories":[
    bcpy(_beginAcc, _beginLen);

    current = accessories;
    while (current != nullptr) {
        // {"aid":xxx,"services":[
        bprintf(_beginAccSrvFmt, current->accessoryIdentifier);

        auto s = current->services;
        while (s != nullptr) {
            tot += _serializeService(bptr, bufLen, s, request, &options);
            s = s->next;
            if (s != nullptr) { bpush(','); }
        }

        // ]}
        bcpy(_end, _endLen);

        current = current->next;
        if (current != nullptr) { bpush(','); }
    }

    // ]}
    bcpy(_end, _endLen);

    request->setContentType(HAP_JSON);
    request->send(buf, bufLen);
    delete[] buf;
}

unsigned int HAPServer::_serializeMultiCharacteristics(
        char * buf,
        unsigned int bufLen,
        const char *idPtr,
        HAPUserHelper * session,
        HAPSerializeOptions * options)
{
    unsigned int tot = 0;

    // {"characteristics":[
    bpstr(_charMultiBegin);

    while (*idPtr != '\x00'){
        char aidStr[8];
        char iidStr[8];
        char * aidPtr = aidStr;
        char * iidPtr = iidStr;
        memset(aidStr, 0, sizeof(aidStr));
        memset(iidStr, 0, sizeof(iidStr));

        while (*idPtr != '.') *aidPtr++ = *idPtr++;
        ++idPtr;
        while (*idPtr != '\x00' && *idPtr != ',') *iidPtr++ = *idPtr++;
        if(*idPtr == ',') ++idPtr;

        unsigned int aid = static_cast<unsigned int>(atoi(aidStr)); // NOLINT
        unsigned int iid = static_cast<unsigned int>(atoi(iidStr)); // NOLINT

        auto c = getCharacteristic(iid, aid);
        if(c){
            options->aid = aid;
            tot += _serializeCharacteristic(bptr, bufLen, c, session, options);
            if(*idPtr != '\x00'){ bpush(','); }
        }
    }

    // ]}
    bpstr(_bEnd);

    return tot;
}

void HAPServer::_handleCharacteristicWrite(HAPUserHelper * request) {
#define tk (tokens[i])
#define tkVal (buf + tk.start)
#define tkLen static_cast<unsigned int>(tk.end - tk.start)
    constexpr uint8_t token_cnt = 64;
    jsmn_parser parser;
    jsmntok_t tokens[token_cnt];
    unsigned int aid;
    unsigned int iid;
    uint8_t flags;
    jsmntok_t * valueToken;
    auto buf = reinterpret_cast<const char *>(request->data());

    jsmn_init(&parser);
    auto tot = jsmn_parse(&parser, buf, request->dataLength(), tokens, token_cnt);

    uint8_t i = 0;
    while (i < tot && tk.type != JSMN_ARRAY) ++i;

    while (i < tot){
        if(tk.type == JSMN_OBJECT){
            aid = 0;
            iid = 0;
            flags = 0;
            valueToken = nullptr;
            auto end = tk.size * 2 + i;
            for(i++; i <= end; ++i){
                if(strncasecmp("aid", tkVal, tkLen) == 0){
                    ++i;
                    aid = static_cast<unsigned int>(atoi(tkVal)); // NOLINT
                } else if (strncasecmp("iid", tkVal, tkLen) == 0){
                    ++i;
                    iid = static_cast<unsigned int>(atoi(tkVal)); // NOLINT
                } else if (strncasecmp("ev", tkVal, tkLen) == 0){
                    ++i;
                    flags |= 0b10;
                    flags &= ~0b01; // NOLINT
                    flags |= *tkVal == 't' || *tkVal == '1' ? 0b01 : 0;
                } else if (strncasecmp("value", tkVal, tkLen) == 0){
                    ++i;
                    valueToken = &tk;
                }
            }

            auto c = getCharacteristic(iid, aid);
            if(!c) continue;

            if(flags & 0b10){ // NOLINT
                if(flags & 0b01) { // NOLINT
                    subscribe(request, c);
                } else { unsubscribe(request, c); }
            }

            if(valueToken){
                CharacteristicValue cVal {};
                switch(c->format){
                    case FORMAT_BOOL: {
                        auto vc = *(buf + valueToken->start);
                        cVal.bool_value = vc == 't' || vc == '1';
                        break;
                    }
                    case FORMAT_INT:
                        cVal.int_value = atoi(buf + valueToken->start); // NOLINT
                        break;
                    default: HAP_DEBUG("Not handing write format %u", c->format);
                }
                c->setValue(cVal, request);
            }
        } else { ++i; }
    }

    request->setResponseStatus(HTTP_204_NO_CONTENT);
    request->send();

#undef tk
}

unsigned int HAPServer::_serializeUpdatedCharacteristics(
        char * buf,
        unsigned int bufLen,
        BaseCharacteristic ** cs,
        HAPUserHelper * session,
        HAPSerializeOptions * options
) {
    unsigned int tot = 0;

    bpstr(_charMultiBegin);

    while (*cs != nullptr){
        tot += _serializeCharacteristic(bptr, bufLen, *cs, session, options);
        ++cs;
        if(*cs) { bpush(','); }
    }

    bpstr(_bEnd);

    return tot;
}

#undef bptr
#undef bpush
#undef bprintf
#undef bcpy
#undef bpprintf
#undef bstr
