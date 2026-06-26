#pragma once

// =====================
// GHOST Identity
// =====================
#define GHOST_NAME "GHOST"

// Temporary test hash (2-byte)
#define GHOST_HASH_1 0x47
#define GHOST_HASH_2 0x48

// =====================
// LoRa Radio Config
// =====================
#define LORA_FREQ       869618000
#define LORA_BW         62500
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