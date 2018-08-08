//
// Created by Xule Zhou on 8/7/18.
//

#include "HomeKitAccessory.h"

SCONST char _serviceBeginFmt[] PROGMEM = R"({"type":"%X","iid":%d,"characteristics":[)";
SCONST char _serviceEnd[] PROGMEM = R"(]})";

SCONST char _typeFmt[] PROGMEM = R"("type":"%X")";
SCONST char _iidFmt[] PROGMEM = R"("iid":%d)";
SCONST char _evFmt[] PROGMEM = R"("ev":%s)";
SCONST char _formatFmt[] PROGMEM = R"("format":"%s")";
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
            PGM_R(fmt, _evFmt);
            tot += snprintf(bptr, bufLen, fmt, isSubscriber(session, c) ? _bool_true : _bool_false);
            bpush(',');
            delete[] fmt;
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

        c = c->next;
        if (c != nullptr) {
            bpush(',');
        }
    }

    PGM_R(serviceEnd, _serviceEnd);
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
            if (s != nullptr) {
                bpush(',');
            }
        }

        // ]}
        bcpy(_end, _endLen);

        current = current->next;
        if (current != nullptr) {
            bpush(',');
        }
    }

    // ]}
    bcpy(_end, _endLen);

    request->setContentType(HAP_JSON);
    request->send(buf, bufLen);
    delete[] buf;
}

#undef bptr
#undef bpush
#undef bprintf
#undef bcpy
#undef bpprintf
#undef bstr
