# Cheap MeshCore Repeater for Heltec CubeCell AB02A

**v0.5 — low-power, MeshCore-compatible repeater for Heltec CubeCell AB02A (SX1262).**

This is an experimental, unofficial project and is not affiliated with MeshCore.
It implements the small set of MeshCore functions needed by a simple repeater,
with an emphasis on fitting and running well on inexpensive, battery-friendly
CubeCell hardware.

## What it does

- Receives and retransmits MeshCore flood traffic.
- Supports trace hops and node discovery requests.
- Sends a MeshCore advert after boot and then every three hours of uptime.
- Can include an optional **manually configured position** in its advert.
  The AB02A has no built-in GPS; position sharing is off by default.
- Suppresses recently seen duplicate floods with a small 12-entry cache.
- Uses a concise 18 s receive / 2 s radio sleep power-save cycle.
- Provides compact 115200-baud serial logs for troubleshooting.

This is deliberately not a full MeshCore repeater firmware: it has no web UI,
Bluetooth, Wi-Fi, GPS receiver, command-line service or broker integration.

## Hardware

- Heltec CubeCell AB02A (CubeCell-1/2AA Node)
- Integrated SX1262 LoRa radio
- USB data cable

## Radio settings

The default configuration is:

| Setting | Value |
| --- | --- |
| Frequency | 869.618 MHz |
| Bandwidth | 62.5 kHz |
| Spreading factor | SF6 |
| Coding rate | 4/8 |
| Sync word | 0x12 |

They must match the MeshCore network you want to join. Frequency, spreading
factor and TX power live in `config.h` (`LORA_FREQ`, `LORA_SF`,
`LORA_TX_POWER`); bandwidth, coding rate and preamble are the fixed MeshCore
SF6 preset in `radio.cpp`.

## Install and build

### 1. Install Arduino IDE and the board package

Install Arduino IDE 2.x. In **Tools → Board → Boards Manager**, install the
CubeCell / ASR650x board support that provides **CubeCell-1/2AA Node
(HTCC-AB02A)**. Open `GHOST.ino` from this repository and select that board.

The project needs no external Arduino libraries — Ed25519 and SHA-512 are
bundled in the repository.

### 2. Create a unique identity

Every repeater needs its own Ed25519 key pair. Generate a MeshCore-compatible
public/private pair with a trusted MeshCore key generator, then keep the
private key in a safe backup location. Never publish it, commit it, or reuse
another node's identity.

From PowerShell in the project directory, run:

```powershell
.\Configure-Identity.ps1
```

Paste the requested values:

- **Public key:** 64 hexadecimal characters
- **Private key:** 128 hexadecimal characters

The helper validates their format and creates `identity.cpp` locally.
`identity.cpp` is intentionally ignored by Git, so it is never included in a
commit or GitHub push.

If you already maintain the file manually, copy `identity.example.txt` to
`identity.cpp` and replace both arrays with your own keys.

### 3. Configure the repeater

Open `config.h` and set the visible repeater name:

```cpp
#define GHOST_NAME "GHOST"
```

Leave the position option disabled unless you deliberately want to publish a
fixed location:

```cpp
#define GHOST_ADVERT_POSITION_ENABLED 0
```

To publish a position, set it to `1` and supply latitude and longitude in
decimal degrees multiplied by 1,000,000:

```cpp
#define GHOST_ADVERT_POSITION_ENABLED 1
#define GHOST_ADVERT_POSITION_LAT_E6 50244361  // 50.244361
#define GHOST_ADVERT_POSITION_LON_E6 19060222  // 19.060222
```

Without a position, advert names are safely limited to 24 characters; with a
position, the limit is 16 characters.

### 4. Compile and upload

In Arduino IDE select:

```text
Board: CubeCell-1/2AA Node (HTCC-AB02A)
Port:  the USB serial port of the CubeCell
```

Click **Upload**. A successful build of v0.5 uses about 103 KB of the AB02A's
128 KB program space.

### 5. Open the serial monitor

Open the Arduino Serial Monitor and select:

```text
Baud rate: 115200
```

Typical output:

```text
[GHOST] boot
[RADIO] ready
[RX] route=1 type=4 hops=3 bytes=134 rssi=-40 snr=10
[TX] flood hops=4 bytes=136
[DROP] duplicate
```

`[DROP] duplicate` is normal: it means the repeater heard the same flood
again and correctly avoided forwarding it twice.

## Behaviour and limits

- The three-hour advert interval is based on `millis()` uptime and is safe
  across its counter rollover. It is not a wall-clock schedule; the project
  has no RTC or real-time source.
- The 18 s RX / 2 s sleep cycle saves power, but the radio cannot receive
  packets during its 2-second sleep window.
- A new key is required for each physical repeater. Back up the private key
  before flashing: it cannot be recovered later.

## Protocol support

Implemented packet handling includes:

- ADVERT
- TRACE
- CONTROL: `CTL_TYPE_NODE_DISCOVER_REQ` (`0x80`) and
  `CTL_TYPE_NODE_DISCOVER_RESP` (`0x92`)

Protocol behaviour was validated against MeshCore traffic, the MeshCore simple
repeater example and live MeshCore Companion testing.

## Release notes

### v0.5

- Direct packets are now relayed exactly like the official MeshCore repeater:
  forwarded only when the first path hash is ours (our hash is then removed
  from the path). Packets addressed elsewhere are dropped instead of blindly
  retransmitted.
- Flood loop protection: a flood whose path already contains our key prefix
  is dropped (backup for the 12-entry duplicate cache, which now also expires
  entries after 5 minutes).
- The periodic advert waits until the radio is idle instead of colliding with
  a retransmission in progress.
- Soft watchdog: if a transmission never completes (radio driver hang), the
  radio is reinitialised automatically. It does not interfere with the
  18 s RX / 2 s sleep power-save cycle.
- Boot refuses to run with the placeholder (all-zero) identity from
  `identity.example.txt`.
- Serial logging can be silenced for battery duty: set `GHOST_DEBUG` to `0`
  in `config.h`.
- Radio settings unified: `config.h` is now the single place for frequency,
  spreading factor and TX power (no more duplicated constants in
  `radio.cpp`).
- Firmware binaries are built by CI and attached to GitHub releases (built
  with the ASR650x 1.6.0 board manager and a zero identity — the CI binary
  boots into a safe halt until you build with your own identity).

### v0.3

- Added safe local identity configuration through `Configure-Identity.ps1`.
- Made advert position optional and documented it as a manual position, not
  GPS.
- Added an advert at boot and every three hours of uptime.
- Added robust packet bounds checks and correct identity-derived path hashes.
- Added flood duplicate suppression and compact serial logging.
- Confirmed build and live serial operation on CubeCell AB02A.

### v0.2

- Added 18 s RX / 2 s sleep power-save cycle.
- Added TX-safe sleep protection and improved forwarding stability.

## License

See [LICENSE](LICENSE).
