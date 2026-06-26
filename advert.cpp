#include "advert.h"
#include "identity.h"
#include "radio.h"
#include "meshcore_sign.h"

GhostAdvert ghostAdvert;

static uint8_t advertPacket[128];
static uint8_t signature[64];

void GhostAdvert::send() {
    Serial.println();
    Serial.println("BUILD ADVERT");

    const char name[] = "GHOST";
    uint32_t ts = 1750679400;

    int32_t lat = 50244361;
    int32_t lon = 19060222;

    uint8_t flags = 0x92; // repeater + location + name

    uint8_t msg[128];
    int m = 0;

    memcpy(msg + m, GHOST_PUBLIC_KEY, 32);
    m += 32;

    memcpy(msg + m, &ts, 4);
    m += 4;

    msg[m++] = flags;

    memcpy(msg + m, &lat, 4);
    m += 4;

    memcpy(msg + m, &lon, 4);
    m += 4;

    memcpy(msg + m, name, 5);
    m += 5;

    meshcore_sign(
        signature,
        GHOST_PRIVATE_KEY,
        GHOST_PUBLIC_KEY,
        msg,
        m
    );

    int p = 0;

    advertPacket[p++] = 0x11; // advert + flood
    advertPacket[p++] = 0x00; // path len

    memcpy(advertPacket + p, GHOST_PUBLIC_KEY, 32);
    p += 32;

    memcpy(advertPacket + p, &ts, 4);
    p += 4;

    memcpy(advertPacket + p, signature, 64);
    p += 64;

    advertPacket[p++] = flags;

    memcpy(advertPacket + p, &lat, 4);
    p += 4;

    memcpy(advertPacket + p, &lon, 4);
    p += 4;

    memcpy(advertPacket + p, name, 5);
    p += 5;

    Serial.print("ADVERT SIZE=");
    Serial.println(p);

    Serial.println("ADVERT TX");
    Radio.Send(advertPacket, p);
}