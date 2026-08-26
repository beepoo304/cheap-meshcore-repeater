#include "advert.h"
#include "config.h"
#include "identity.h"
#include "radio.h"
#include "meshcore_sign.h"

GhostAdvert ghostAdvert;

static uint8_t advertPacket[128];
static uint8_t signature[64];

namespace {

const uint8_t kAdvertHeader = 0x11;  // advert + flood
const uint8_t kEmptyPath = 0x00;
const uint32_t kAdvertTimestamp = 1750679400UL;
#if GHOST_ADVERT_POSITION_ENABLED
const size_t kMaxAdvertNameLength = 16;
#else
const size_t kMaxAdvertNameLength = 24;
#endif

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
    const uint32_t timestamp = kAdvertTimestamp;
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
