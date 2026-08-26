#pragma once

#include "Arduino.h"

#define MAX_PACKET_SIZE 255
#define MAX_PATH_SIZE 128
#define MAX_PACKET_PAYLOAD 200

#define PH_ROUTE_MASK     0x03
#define PH_TYPE_SHIFT     2
#define PH_TYPE_MASK      0x0F
#define PH_VER_SHIFT      6
#define PH_VER_MASK       0x03

#define ROUTE_TYPE_TRANSPORT_FLOOD   0x00
#define ROUTE_TYPE_FLOOD             0x01
#define ROUTE_TYPE_DIRECT            0x02
#define ROUTE_TYPE_TRANSPORT_DIRECT  0x03

#define PAYLOAD_TYPE_REQ        0x00
#define PAYLOAD_TYPE_RESPONSE   0x01
#define PAYLOAD_TYPE_ACK        0x03
#define PAYLOAD_TYPE_ADVERT     0x04
#define PAYLOAD_TYPE_PATH       0x08
#define PAYLOAD_TYPE_TRACE      0x09
#define PAYLOAD_TYPE_CONTROL    0x0B

class GhostPacket {
public:
    GhostPacket();

    bool parse(uint8_t* data, uint16_t size);
    bool appendOwnHash();

    uint8_t getRouteType();
    uint8_t getPayloadType();
    uint8_t getPayloadVer();

    uint8_t getPathHashSize();
    uint8_t getPathHashCount();
    uint8_t getPathByteLen();

    uint8_t* getPayload();
    uint16_t getPayloadSize();

    uint8_t* getRaw();
    uint16_t getSize();

private:
    uint8_t raw[MAX_PACKET_SIZE];
    uint16_t packetSize;

    uint8_t header;
    uint8_t pathLen;
};
