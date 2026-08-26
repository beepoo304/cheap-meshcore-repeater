#include <Crypto.h>
#include <Ed25519.h>

#include "config.h"
#include "packet.h"
#include "radio.h"
#include "identity.h"
#include "advert.h"

GhostPacket pkt;

uint8_t signature[64];
const char testMsg[] = "GHOST TEST";
static unsigned long lastAdvertAt = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("GHOST BOOT");
  Serial.println("----------");

  Serial.print("HASH=");
  Serial.print(GHOST_PUBLIC_KEY[0], HEX);
  Serial.print(GHOST_PUBLIC_KEY[1], HEX);
  Serial.println();

  uint8_t priv32[32];
  memcpy(priv32, GHOST_PRIVATE_KEY, 32);

  Ed25519::sign(
    signature,
    priv32,
    GHOST_PUBLIC_KEY,
    testMsg,
    strlen(testMsg)
  );

  Serial.println("SIGN OK");

  for (int i = 0; i < 64; i++) {
    if (signature[i] < 16) Serial.print("0");
    Serial.print(signature[i], HEX);
  }

  Serial.println();

  ghostRadio.begin();

  delay(3000);
  ghostAdvert.send();
  lastAdvertAt = millis();
}

void loop() {
  ghostRadio.update();

  if (millis() - lastAdvertAt >= GHOST_ADVERT_INTERVAL_MS) {
    ghostAdvert.send();
    lastAdvertAt = millis();
  }
}
