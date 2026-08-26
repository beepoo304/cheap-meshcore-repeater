#include "packet.h"
#include "identity.h"

GhostPacket::GhostPacket() {
    packetSize = 0;
    header = 0;
    pathLen = 0;
}

bool GhostPacket::parse(uint8_t* data, uint16_t size) {
    if (size < 2) return false;
    if (size > MAX_PACKET_SIZE) return false;

    memcpy(raw, data, size);
    packetSize = size;

    header = raw[0];

    uint8_t routeType = getRouteType();
    uint8_t offset = 1;

    if (routeType == ROUTE_TYPE_TRANSPORT_FLOOD ||
        routeType == ROUTE_TYPE_TRANSPORT_DIRECT) {
        offset += 4;
    }

    if (offset >= size) return false;

    pathLen = raw[offset];
    const uint16_t pathBytes = getPathByteLen();
    if (static_cast<uint16_t>(offset + 1U + pathBytes) > size) return false;
    return true;
}

uint8_t GhostPacket::getRouteType() {
    return header & PH_ROUTE_MASK;
}

uint8_t GhostPacket::getPayloadType() {
    return (header >> PH_TYPE_SHIFT) & PH_TYPE_MASK;
}

uint8_t GhostPacket::getPayloadVer() {
    return (header >> PH_VER_SHIFT) & PH_VER_MASK;
}

uint8_t GhostPacket::getPathHashSize() {
    return ((pathLen >> 6) & 0x03) + 1;
}

uint8_t GhostPacket::getPathHashCount() {
    return pathLen & 0x3F;
}

uint8_t GhostPacket::getPathByteLen() {
    return getPathHashSize() * getPathHashCount();
}

uint8_t* GhostPacket::getPayload() {
    uint16_t offset = 2;

    if (getRouteType() == ROUTE_TYPE_TRANSPORT_FLOOD ||
        getRouteType() == ROUTE_TYPE_TRANSPORT_DIRECT) {
        offset += 4;
    }

    offset += getPathByteLen();

    if (offset >= packetSize) return NULL;

    return &raw[offset];
}

uint16_t GhostPacket::getPayloadSize() {
    uint16_t offset = 2;

    if (getRouteType() == ROUTE_TYPE_TRANSPORT_FLOOD ||
        getRouteType() == ROUTE_TYPE_TRANSPORT_DIRECT) {
        offset += 4;
    }

    offset += getPathByteLen();

    if (offset >= packetSize) return 0;

    return packetSize - offset;
}

uint8_t* GhostPacket::getRaw() {
    return raw;
}

uint16_t GhostPacket::getSize() {
    return packetSize;
}

bool GhostPacket::appendOwnHash() {
    uint8_t hs = getPathHashSize();
    uint8_t count = getPathHashCount();

    if (hs < 1 || hs > 3) return false;
    if (count >= 0x3F) return false;

    uint8_t routeType = getRouteType();
    uint16_t insertPos = 2;

    if (routeType == ROUTE_TYPE_TRANSPORT_FLOOD ||
        routeType == ROUTE_TYPE_TRANSPORT_DIRECT) {
        insertPos += 4;
    }

    insertPos += (count * hs);

    if (insertPos > packetSize || (packetSize + hs) > MAX_PACKET_SIZE) {
        return false;
    }

    memmove(raw + insertPos + hs, raw + insertPos, packetSize - insertPos);

    // MeshCore paths contain a prefix of the actual public key. Deriving it
    // here keeps forwarding and loop detection correct for every identity.
    memcpy(raw + insertPos, GHOST_PUBLIC_KEY, hs);

    count++;
    pathLen = (pathLen & 0xC0) | count;

    uint16_t pathLenPos = 1;
    if (routeType == ROUTE_TYPE_TRANSPORT_FLOOD ||
        routeType == ROUTE_TYPE_TRANSPORT_DIRECT) {
        pathLenPos += 4;
    }

    raw[pathLenPos] = pathLen;
    packetSize += hs;

    return true;
}
