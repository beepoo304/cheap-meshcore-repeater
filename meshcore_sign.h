#pragma once
#include <Arduino.h>

void meshcore_sign(
    uint8_t signature[64],
    const uint8_t privateKey[64],
    const uint8_t publicKey[32],
    const void *message,
    size_t len
);