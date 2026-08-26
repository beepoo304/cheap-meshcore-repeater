#include "radio.h"
#include "packet.h"
#include "identity.h"

extern GhostPacket pkt;

RadioEvents_t GhostRadio::RadioEvents;
GhostRadio ghostRadio;

static unsigned long txBlockUntil = 0;

static bool radioSleeping = false;
static bool radioBusy = false;
static unsigned long cycleTimer = 0;

#define FLOOD_DEDUP_CACHE_SIZE 12
static uint32_t floodDedupCache[FLOOD_DEDUP_CACHE_SIZE];
static bool floodDedupUsed[FLOOD_DEDUP_CACHE_SIZE];
static uint8_t floodDedupNext = 0;

#define RX_ON_MS   18000
#define RX_OFF_MS   2000

#define RF_FREQUENCY          869618000
#define TX_OUTPUT_POWER       10

#define LORA_BANDWIDTH        3
#define LORA_SPREADING_FACTOR 6
#define LORA_CODINGRATE       4
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
    for (uint8_t index = 0; index < FLOOD_DEDUP_CACHE_SIZE; ++index) {
        if (floodDedupUsed[index] && floodDedupCache[index] == fingerprint) {
            return true;
        }
    }

    floodDedupCache[floodDedupNext] = fingerprint;
    floodDedupUsed[floodDedupNext] = true;
    floodDedupNext = (floodDedupNext + 1) % FLOOD_DEDUP_CACHE_SIZE;
    return false;
}

bool GhostRadio::begin() {
    Serial.println("[RADIO] init");

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

    Serial.println("[RADIO] ready");
    return true;
}

void GhostRadio::update() {
    Radio.IrqProcess();

    if (radioBusy) return;

    unsigned long now = millis();

    if (!radioSleeping) {
        if (now - cycleTimer >= RX_ON_MS) {
            Serial.println("[RADIO] sleep");
            Radio.Sleep();
            radioSleeping = true;
            cycleTimer = now;
        }
    } else {
        if (now - cycleTimer >= RX_OFF_MS) {
            Serial.println("[RADIO] wake");
            Radio.Rx(0);
            radioSleeping = false;
            cycleTimer = now;
        }
    }
}

void GhostRadio::onTxDone(void) {
    radioBusy = false;
    cycleTimer = millis();
    Serial.println("[TX] done");
    Radio.Rx(0);
}

void GhostRadio::onTxTimeout(void) {
    radioBusy = false;
    cycleTimer = millis();
    Serial.println("[TX] timeout");
    Radio.Rx(0);
}

void GhostRadio::onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
    if (millis() < txBlockUntil) {
        Serial.println("[DROP] cooldown");
        Radio.Rx(0);
        return;
    }

    if (!pkt.parse(payload, size)) {
        Serial.println("[DROP] parse");
        Radio.Rx(0);
        return;
    }

    uint8_t routeType = pkt.getRouteType();
    uint8_t payloadType = pkt.getPayloadType();
    uint8_t hops = pkt.getPathHashCount();

    Serial.print("[RX] route=");
    Serial.print(routeType);
    Serial.print(" type=");
    Serial.print(payloadType);
    Serial.print(" hops=");
    Serial.print(hops);
    Serial.print(" bytes=");
    Serial.print(size);
    Serial.print(" rssi=");
    Serial.print(rssi);
    Serial.print(" snr=");
    Serial.println(snr);

    if (routeType == ROUTE_TYPE_FLOOD && isDuplicateFlood(pkt)) {
        Serial.println("[DROP] duplicate");
        Radio.Rx(0);
        return;
    }

    if (routeType == ROUTE_TYPE_DIRECT &&
        payloadType == PAYLOAD_TYPE_CONTROL) {

        uint8_t* p = pkt.getPayload();

        if (p && pkt.getPayloadSize() >= 6 && p[0] == 0x80) {
            Serial.println("[DISCOVER] reply");

            uint8_t resp[40];
            resp[0] = 0x2E;
            resp[1] = 0x00;
            resp[2] = 0x92;
            resp[3] = (uint8_t)(snr * 4);

            memcpy(&resp[4], &p[2], 4);
            memcpy(&resp[8], GHOST_PUBLIC_KEY, 32);

            txBlockUntil = millis() + 250;
            radioBusy = true;
            Radio.Send(resp, sizeof(resp));
            return;
        }

        txBlockUntil = millis() + 250;
        radioBusy = true;
        Serial.print("[TX] direct bytes=");
        Serial.println(pkt.getSize());
        Radio.Send(pkt.getRaw(), pkt.getSize());
        return;
    }

    if (hops >= 8) {
        Serial.println("[DROP] max-hops");
        Radio.Rx(0);
        return;
    }

    if (!pkt.appendOwnHash()) {
        Serial.println("[DROP] append");
        Radio.Rx(0);
        return;
    }

    txBlockUntil = millis() + 250;
    radioBusy = true;
    Serial.print("[TX] flood hops=");
    Serial.print(pkt.getPathHashCount());
    Serial.print(" bytes=");
    Serial.println(pkt.getSize());
    Radio.Send(pkt.getRaw(), pkt.getSize());
}
