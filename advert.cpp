#include "advert.h"
#include "config.h"
#include "identity.h"
#include "radio.h"
#include "meshcore_sign.h"

#include <string.h>
#include <stdint.h>

GhostAdvert ghostAdvert;

static uint8_t advertPacket[128];
static uint8_t signature[64];

namespace {

const uint8_t kAdvertHeader = 0x11;  // advert + flood
const uint8_t kEmptyPath = 0x00;

#if GHOST_ADVERT_POSITION_ENABLED
const size_t kMaxAdvertNameLength = 16;
#else
const size_t kMaxAdvertNameLength = 24;
#endif

// Convert the compiler's build date and time to Unix timestamp.
uint32_t buildTimestamp() {
    const char* date = __DATE__;
    const char* time = __TIME__;

    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    unsigned int month = 1;

    for (unsigned int i = 0; i < 12; ++i) {
        if (memcmp(date, months + i * 3, 3) == 0) {
            month = i + 1;
            break;
        }
    }

    unsigned int day = 0;
    if (date[4] >= '0' && date[4] <= '9') {
        day = (unsigned int)(date[4] - '0') * 10;
    }
    day += (unsigned int)(date[5] - '0');

    int year =
        (date[7] - '0') * 1000 +
        (date[8] - '0') * 100 +
        (date[9] - '0') * 10 +
        (date[10] - '0');

    unsigned int hour =
        (unsigned int)(time[0] - '0') * 10 +
        (unsigned int)(time[1] - '0');

    unsigned int minute =
        (unsigned int)(time[3] - '0') * 10 +
        (unsigned int)(time[4] - '0');

    unsigned int second =
        (unsigned int)(time[6] - '0') * 10 +
        (unsigned int)(time[7] - '0');

    int adjustedYear = year;
    if (month <= 2) {
        --adjustedYear;
    }

    const int era =
        (adjustedYear >= 0 ? adjustedYear : adjustedYear - 399) / 400;

    const unsigned int yearOfEra =
        (unsigned int)(adjustedYear - era * 400);

    const unsigned int adjustedMonth =
        month > 2 ? month - 3 : month + 9;

    const unsigned int dayOfYear =
        (153 * adjustedMonth + 2) / 5 + day - 1;

    const unsigned int dayOfEra =
        yearOfEra * 365 +
        yearOfEra / 4 -
        yearOfEra / 100 +
        dayOfYear;

    const int daysSinceEpoch =
        era * 146097 + (int)dayOfEra - 719468;

    return (uint32_t)daysSinceEpoch * 86400UL
         + hour * 3600UL
         + minute * 60UL
         + second;
}

uint8_t advertFlags() {
    uint8_t flags = GHOST_ADVERT_TYPE_REPEATER | GHOST_ADVERT_FLAG_NAME;
#if GHOST_ADVERT_POSITION_ENABLED
    flags |= GHOST_ADVERT_FLAG_POSITION;
#endif
    return flags;
}

void appendPosition(uint8_t* buffer, int& offset) {
#if GHOST_ADVERT_POSITION_ENABLED
    const int32_t lat = GHOST_ADVERT_POSITION_LAT_E6;
    const int32_t lon = GHOST_ADVERT_POSITION_LON_E6;

    memcpy(buffer + offset, &lat, sizeof(lat));
    offset += sizeof(lat);

    memcpy(buffer + offset, &lon, sizeof(lon));
    offset += sizeof(lon);
#else
    (void)buffer;
    (void)offset;
#endif
}

size_t advertNameLength(const char* name) {
    size_t length = 0;

    while (name[length] != '\0' && length < kMaxAdvertNameLength) {
        ++length;
    }

    return length;
}

}  // namespace

void GhostAdvert::send() {
    const char* name = GHOST_NAME;

    // Build time + uptime: no RTC or network time required.
    const uint32_t timestamp = buildTimestamp() + (millis() / 1000UL);

    const uint8_t flags = advertFlags();

    uint8_t msg[128];
    int m = 0;

    memcpy(msg + m, GHOST_PUBLIC_KEY, 32);
    m += 32;

    memcpy(msg + m, &timestamp, sizeof(timestamp));
    m += sizeof(timestamp);

    msg[m++] = flags;

    appendPosition(msg, m);

    const size_t nameLength = advertNameLength(name);
    memcpy(msg + m, name, nameLength);
    m += nameLength;

    meshcore_sign(
        signature,
        GHOST_PRIVATE_KEY,
        GHOST_PUBLIC_KEY,
        msg,
        m
    );

    int p = 0;

    advertPacket[p++] = kAdvertHeader;
    advertPacket[p++] = kEmptyPath;

    memcpy(advertPacket + p, GHOST_PUBLIC_KEY, 32);
    p += 32;

    memcpy(advertPacket + p, &timestamp, sizeof(timestamp));
    p += sizeof(timestamp);

    memcpy(advertPacket + p, signature, 64);
    p += 64;

    advertPacket[p++] = flags;

    appendPosition(advertPacket, p);

    memcpy(advertPacket + p, name, nameLength);
    p += nameLength;

    Radio.Send(advertPacket, p);
}
