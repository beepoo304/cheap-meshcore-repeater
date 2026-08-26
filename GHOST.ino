#include "config.h"
#include "packet.h"
#include "radio.h"
#include "identity.h"
#include "advert.h"

GhostPacket pkt;

static unsigned long lastAdvertAt = 0;

void setup() {
  Serial.begin(115200);
  delay(3000);

  Serial.println();
  Serial.println("[GHOST] boot");

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

  if (millis() - lastAdvertAt >= GHOST_ADVERT_INTERVAL_MS) {
    ghostAdvert.send();
    lastAdvertAt = millis();
  }
}
