#pragma once

#include "LoRaWan_APP.h"
#include "Arduino.h"

class GhostRadio {
public:
    bool begin();
    void update();

private:
    static RadioEvents_t RadioEvents;

    static void onTxDone(void);
    static void onTxTimeout(void);
    static void onRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr);
};

extern GhostRadio ghostRadio;