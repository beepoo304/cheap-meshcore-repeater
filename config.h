#pragma once

#include <Arduino.h>

// ============================================================================
// USER CONFIGURATION
// Change these values before building a firmware for your own repeater.
// ============================================================================

// Repeater name sent in MeshCore adverts.
#define GHOST_NAME "GHOST"

// Set to 0 to silence all serial logging (saves power in production).
#define GHOST_DEBUG 1

#if GHOST_DEBUG
#define GLOG(...)    do { Serial.print(__VA_ARGS__); } while (0)
#define GLOGLN(...)  do { Serial.println(__VA_ARGS__); } while (0)
#else
#define GLOG(...)    do { } while (0)
#define GLOGLN(...)  do { } while (0)
#endif

// Optional advert position. Set ENABLED to 1 only when you intentionally want
// to share a manually configured position. This project has no GPS hardware.
// LAT/LON are signed decimal degrees * 1,000,000.
// 50°16'48.7"N 19°02'07.2"E = 50.280194, 19.035333
#define GHOST_ADVERT_POSITION_ENABLED 1
#define GHOST_ADVERT_POSITION_LAT_E6 50280194
#define GHOST_ADVERT_POSITION_LON_E6 19035333

// ============================================================================
// PROTOCOL CONSTANTS -- do not change unless updating MeshCore compatibility.
// ============================================================================
// MeshCore advert-data flags. Keep these local constants named rather than
// scattering protocol literals through the advert encoder.
#define GHOST_ADVERT_TYPE_REPEATER 0x02
#define GHOST_ADVERT_FLAG_POSITION 0x10
#define GHOST_ADVERT_FLAG_NAME 0x80

// The device has no clock source. Broadcast the signed advert once at boot
// and then on this fixed millis()-based interval.
#define GHOST_ADVERT_INTERVAL_MS (3UL * 60UL * 60UL * 1000UL)

// =====================
// LoRa Radio Config
// =====================
#define LORA_FREQ       869618000UL
#define LORA_SF         6
#define LORA_TX_POWER   10

// =====================
// MeshCore Config
// =====================
#define PATH_HASH_SIZE      2
#define LOOP_DETECT_MINIMAL 1

// =====================
// Packet Limits (authoritative copy lives in packet.h)
// =====================

