#include "radio.h"
#include "packet.h"
#include "identity.h"
#include "config.h"

extern GhostPacket pkt;

RadioEvents_t GhostRadio::RadioEvents;
GhostRadio ghostRadio;

static unsigned long txBlockUntil = 0;

static bool radioSleeping = false;
static bool radioBusy = false;
static unsigned long cycleTimer = 0;

// Soft watchdog: if a TX never completes (radio driver hang), reinit the
// radio instead of waiting for a manual reset.
static unsigned long txStartedAt = 0;
#define TX_HANG_MS 60000UL

#define FLOOD_DEDUP_CACHE_SIZE 12
#define FLOOD_DEDUP_TTL_MS 300000UL   // entries expire after 5 minutes
static uint32_t floodDedupCache[FLOOD_DEDUP_CACHE_SIZE];
static unsigned long floodDedupAt[FLOOD_DEDUP_CACHE_SIZE];
static bool floodDedupUsed[FLOOD_DEDUP_CACHE_SIZE];
static uint8_t floodDedupNext = 0;

#define RX_ON_MS   18000
#define RX_OFF_MS   2000

#define RF_FREQUENCY          LORA_FREQ
#define TX_OUTPUT_POWER       LORA_TX_POWER

#define LORA_BANDWIDTH        3   // 62.5 kHz (MeshCore SF6 preset, driver enum)
#define LORA_SPREADING_FACTOR LORA_SF
#define LORA_CODINGRATE       4   // 4/8 (MeshCore SF6 preset)
#define LORA_PREAMBLE_LENGTH  16
#define LORA_SYMBOL_TIMEOUT   5

#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false

static uint32_t fingerprintFlood(const uint8_t* payload, uint16_t size,
                                 uint8_t header) {
    uint32_t hash = 2166136261UL;  // FNV-1a: compact, deterministic hash.
    hash ^= header;
    hash *= 16777619UL;

    for (uint16_t index = 0; index < size; ++index) {
        hash ^= payload[index];
        hash *= 16777619UL;
    }

    return hash;
}

static bool isDuplicateFlood(GhostPacket& packet) {
    const uint8_t* payload = packet.getPayload();
    const uint16_t size = packet.getPayloadSize();
    if (payload == NULL || size == 0) {
        return false;
    }

    const uint32_t fingerprint =
        fingerprintFlood(payload, size, packet.getRaw()[0]);
    const unsigned long now = millis();

    for (uint8_t index = 0; index < FLOOD_DEDUP_CACHE_SIZE; ++index) {
        if (floodDedupUsed[index] && floodDedupCache[index] == fingerprint) {
            if (now - floodDedupAt[index] < FLOOD_DEDUP_TTL_MS) {
                return true;  // seen recently: duplicate
            }
            floodDedupUsed[index] = false;  // stale entry: treat as new
        }
    }

    floodDedupCache[floodDedupNext] = fingerprint;
    floodDedupAt[floodDedupNext] = now;
    floodDedupUsed[floodDedupNext] = true;
    floodDedupNext = (floodDedupNext + 1) % FLOOD_DEDUP_CACHE_SIZE;
    return false;
}

bool GhostRadio::begin() {
    GLOGLN("[RADIO] init");

    RadioEvents.TxDone = onTxDone;
    RadioEvents.TxTimeout = onTxTimeout;
    RadioEvents.RxDone = onRxDone;

    Radio.Init(&RadioEvents);
    Radio.SetChannel(RF_FREQUENCY);
    Radio.SetSyncWord(0x12);

    Radio.SetTxConfig(
        MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
        LORA_SPREADING_FACTOR, LORA_CODINGRATE,
        LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
        true, 0, 0, LORA_IQ_INVERSION_ON, 3000
    );

    Radio.SetRxConfig(
        MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
        LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
        LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
        0, true, 0, 0, LORA_IQ_INVERSION_ON, true
    );

    Radio.Rx(0);
    cycleTimer = millis();

    GLOGLN("[RADIO] ready");
    return true;
}

bool GhostRadio::isBusy() { return radioBusy; }

void GhostRadio::update() {
    Radio.IrqProcess();

    // Soft watchdog: a TX that never completes means the radio driver hung.
    // Reinit the radio instead of staying dead until a manual reset.
    if (radioBusy && millis() - txStartedAt > TX_HANG_MS) {
        Serial.println("[RADIO] TX hang, reinit");
        begin();
        return;
    }

    if (radioBusy) return;

    unsigned long now = millis();

    if (!radioSleeping) {
        if (now - cycleTimer >= RX_ON_MS) {
            GLOGLN("[RADIO] sleep");
            Radio.Sleep();
            radioSleeping = true;
            cycleTimer = now;
        }
    } else {
        if (now - cycleTimer >= RX_OFF_MS) {
            GLOGLN("[RADIO] wake");
            Radio.Rx(0);
            radioSleeping = false;
            cycleTimer = now;
        }
    }
}

void GhostRadio::onTxDone(void) {
    radioBusy = false;
    cycleTimer = millis();
    GLOGLN("[TX] done");
    Radio.Rx(0);
}

void GhostRadio::onTxTimeout(void) {
    radioBusy = false;
    cycleTimer = millis();
    GLOGLN("[TX] timeout");
    Radio.Rx(0);
}

void GhostRadio::onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (millis() < txBlockUntil) {
        GLOGLN("[DROP] cooldown");
        Radio.Rx(0);
        return;
    }

    if (!pkt.parse(payload, size)) {
        GLOGLN("[DROP] parse");
        Radio.Rx(0);
        return;
    }

    uint8_t routeType = pkt.getRouteType();
    uint8_t payloadType = pkt.getPayloadType();
    uint8_t hops = pkt.getPathHashCount();

    GLOG("[RX] route=");
    GLOG(routeType);
    GLOG(" type=");
    GLOG(payloadType);
    GLOG(" hops=");
    GLOG(hops);
    GLOG(" bytes=");
    GLOG(size);
    GLOG(" rssi=");
    GLOG(rssi);
    GLOG(" snr=");
    GLOGLN(snr);

    // Loop prevention (MeshCore style): if our key prefix is already in the
    // path, this flood already passed through us. Drop it.
    if ((routeType == ROUTE_TYPE_FLOOD ||
         routeType == ROUTE_TYPE_TRANSPORT_FLOOD) && pkt.pathHasOwnPrefix()) {
        GLOGLN("[DROP] loop");
        Radio.Rx(0);
        return;
    }

    if ((routeType == ROUTE_TYPE_FLOOD ||
         routeType == ROUTE_TYPE_TRANSPORT_FLOOD) && isDuplicateFlood(pkt)) {
        GLOGLN("[DROP] duplicate");
        Radio.Rx(0);
        return;
    }

    if (routeType == ROUTE_TYPE_DIRECT ||
        routeType == ROUTE_TYPE_TRANSPORT_DIRECT) {

        uint8_t* p = pkt.getPayload();

        // MeshCore: zero-hop CONTROL packets with the 0x80 flag (discovery
        // family) are processed locally, never relayed. Only a request that
        // reaches us directly (empty path) gets a reply.
        if (payloadType == PAYLOAD_TYPE_CONTROL && p &&
            pkt.getPayloadSize() >= 6 && (p[0] & 0x80) != 0) {

            if (pkt.getPathHashCount() == 0) {
                GLOGLN("[DISCOVER] reply");

                uint8_t resp[40];
                resp[0] = 0x2E;
                resp[1] = 0x00;
                resp[2] = 0x92;
                resp[3] = (uint8_t)(snr * 4);

                memcpy(&resp[4], &p[2], 4);
                memcpy(&resp[8], GHOST_PUBLIC_KEY, 32);

                txBlockUntil = millis() + 250;
                txStartedAt = millis();
                radioBusy = true;
                Radio.Send(resp, sizeof(resp));
            } else {
                GLOGLN("[DROP] control-relayed");
                Radio.Rx(0);
            }
            return;
        }

        // Direct relay (MeshCore style): forward only when the FIRST hash in
        // the path is ours; then remove our hash and retransmit. Packets
        // addressed elsewhere are dropped, not blindly retransmitted.
        if (pkt.getPathHashCount() > 0 && pkt.popOwnHash()) {
            GLOG("[TX] direct-relay bytes=");
            GLOGLN(pkt.getSize());

            txBlockUntil = millis() + 250;
            txStartedAt = millis();
            radioBusy = true;
            Radio.Send(pkt.getRaw(), pkt.getSize());
            return;
        }

        GLOGLN("[DROP] not-for-us");
        Radio.Rx(0);
        return;
    }

    if (hops >= 8) {
        GLOGLN("[DROP] max-hops");
        Radio.Rx(0);
        return;
    }

    if (!pkt.appendOwnHash()) {
        GLOGLN("[DROP] append");
        Radio.Rx(0);
        return;
    }

    txBlockUntil = millis() + 250;
    txStartedAt = millis();
    radioBusy = true;
    GLOG("[TX] flood hops=");
    GLOG(pkt.getPathHashCount());
    GLOG(" bytes=");
    GLOGLN(pkt.getSize());
    Radio.Send(pkt.getRaw(), pkt.getSize());
}
