# Cheap MeshCore Repeater for Heltec CubeCell AB02A

Current release: **v0.2**
Status: **stable**

## Disclaimer
This is an experimental unofficial project and is not affiliated with MeshCore.

Ultra-lightweight MeshCore-compatible repeater for Heltec CubeCell AB02A (SX1262).

Experimental and unofficial MeshCore-compatible implementation designed for cheap, low-power and solar deployments.

---

## Features

* MeshCore flood retransmission
* Trace hop support
* MeshCore advert support
* Discover nearby nodes support
* Very low resource usage

---

## Hardware

* Heltec CubeCell AB02A
* SX1262 radio

---

## Radio Settings

Current build is configured for:

* Frequency: 869.618 MHz
* Bandwidth: 62.5 kHz
* Spreading Factor: SF6
* Coding Rate: 4/8
* SyncWord: 0x12

These settings must match your MeshCore network.

---

## Project Goal

This project was created as a minimal repeater implementation for MeshCore networks.

Unlike standard repeaters running the full MeshCore stack, this implementation only supports the protocol features required for repeater operation:

* packet forwarding
* trace routing
* advert broadcasting
* node discovery

This keeps memory usage low and makes deployment possible on cheap hardware.

---

## Protocol Support

Implemented packet handling:

* ADVERT
* TRACE
* CONTROL (discover)

Implemented control flow:

* CTL_TYPE_NODE_DISCOVER_REQ (0x80)
* CTL_TYPE_NODE_DISCOVER_RESP (0x92)

---

## Based On

Protocol behavior was analyzed using:

* MeshCore source code
* MeshCore simple_repeater example
* RF packet inspection
* Live testing with MeshCore Companion

Some protocol behavior was reverse engineered from real MeshCore traffic.

---

## Identity Setup

Create:

identity.cpp

by copying:

identity.example.txt

Rename the copied file to `identity.cpp`, then replace:

* GHOST_PUBLIC_KEY
* GHOST_PRIVATE_KEY

with your own keys.

identity.cpp is intentionally excluded from git.

---

## Configuration

Before build, adjust:

* identity.cpp
* config.h
* radio.cpp

Important settings:

* keys
* repeater name
* radio settings

### Optional position in adverts

This project has no GPS hardware. Position is an optional, manually configured
field in the MeshCore advert. A freshly built repeater sends a valid advert
with its name but without a position. To publish a fixed repeater position,
edit `config.h`:

```cpp
#define GHOST_ADVERT_POSITION_ENABLED 1
#define GHOST_ADVERT_POSITION_LAT_E6 50244361  // latitude 50.244361
#define GHOST_ADVERT_POSITION_LON_E6 19060222  // longitude 19.060222
```

Coordinates are signed decimal degrees multiplied by 1,000,000. Keep
`GHOST_ADVERT_POSITION_ENABLED` set to `0` when no position should be shared.
The advert name is safely limited to 24 characters without a position, or 16
characters when a position is enabled.

---

## Build

Tested with:

* Arduino IDE 1.8.x
* CubeCell AB02A board package

---

## Current Status

Working:

* Flood retransmission
* Trace routing
* Advert TX
* Discover nearby nodes

Tested successfully with MeshCore Companion.

---

## Disclaimer

This is an experimental project and is not an official MeshCore implementation.
