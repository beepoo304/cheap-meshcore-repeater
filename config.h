#pragma once

// ============================================================================
// USER CONFIGURATION
// Change these values before building a firmware for your own repeater.
// ============================================================================

// Repeater name sent in MeshCore adverts.
#define GHOST_NAME "GHOST"

// Optional advert position. Set ENABLED to 1 only when you intentionally want
// to share a manually configured position. This project has no GPS hardware.
// LAT/LON are signed decimal degrees * 1,000,000.
// Example: 50.244361, 19.060222 becomes 50244361, 19060222.
// Leave disabled to send a valid advert without any location field.
#define GHOST_ADVERT_POSITION_ENABLED 0
#define GHOST_ADVERT_POSITION_LAT_E6 0
#define GHOST_ADVERT_POSITION_LON_E6 0

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
#define LORA_BW         62500UL
#define LORA_SF         6
#define LORA_CR         8
#define LORA_TX_POWER   10

// =====================
// MeshCore Config
// =====================
#define PATH_HASH_SIZE      2
#define LOOP_DETECT_MINIMAL 1

// =====================
// Packet Limits
// =====================
#define MAX_PACKET_SIZE 256
#define MAX_PATH_SIZE   128
