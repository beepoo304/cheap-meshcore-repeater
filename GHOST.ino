#include "config.h"
#include "packet.h"
#include "radio.h"
#include "identity.h"
#include "advert.h"

#define GHOST_VERSION "0.5.0"

GhostPacket pkt;

static unsigned long lastAdvertAt = 0;

static bool allZero(const uint8_t* data, uint8_t len) {
    for (uint8_t i = 0; i < len; ++i) {
        if (data[i] != 0) return false;
    }
    return true;
}

void setup() {
    Serial.begin(115200);
    delay(3000);

    Serial.println();
    Serial.print("[GHOST] boot v");
    Serial.println(GHOST_VERSION);

    // Refuse to run with the placeholder (all-zero) identity: a repeater with
    // an unconfigured key would pollute the network and cannot be recovered.
    if (allZero(GHOST_PUBLIC_KEY, 8) || allZero(GHOST_PRIVATE_KEY, 8)) {
        Serial.println("[GHOST] BLAD: identity.cpp zawiera przykladowe (zerowe) klucze!");
        Serial.println("[GHOST] Uruchom Configure-Identity.ps1 i wgraj firmware ponownie.");
        Serial.println("[GHOST] Zatrzymano.");
        while (true) {
            delay(10000);
        }
    }

    Serial.print("[GHOST] hash=");
    Serial.print(GHOST_PUBLIC_KEY[0], HEX);
    Serial.print(GHOST_PUBLIC_KEY[1], HEX);
    Serial.println();

    ghostRadio.begin();

    delay(3000);
    ghostAdvert.send();
    lastAdvertAt = millis();
}

void loop() {
    ghostRadio.update();

    // Advert: wait while the radio is mid-transmission, never collide.
    if (millis() - lastAdvertAt >= GHOST_ADVERT_INTERVAL_MS) {
        if (!ghostRadio.isBusy()) {
            ghostAdvert.send();
            lastAdvertAt = millis();
        }
    }
}
