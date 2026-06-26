#include <Arduino.h>
#include "meshcore_sign.h"

extern "C" {
#include "ed_25519.h"
}

void meshcore_sign(
    uint8_t signature[64],
    const uint8_t privateKey[64],
    const uint8_t publicKey[32],
    const void *message,
    size_t len
) {
    ed25519_sign(
        signature,
        (const unsigned char*)message,
        len,
        publicKey,
        privateKey
    );
}