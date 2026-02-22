# Hyperk

Hyperk is a minimalist, high-performance uni-platform WiFi LED driver for ESP8266, ESP32 (S2, S3, C3, C6), Raspberry Pi Pico W (RP2040, RP2350). Designed as a lightweight and streamlined component that avoids unnecessary complexity, it delivers low‑latency performance and integrates smoothly with platforms such as HyperHDR, while offering essential home‑automation capabilities through a clean, modern codebase.

## Installation

The firmware can be flashed directly from your browser:
**[hyperk.hyperhdr.org](https://hyperk.hyperhdr.org)**

> [!TIP]
> Once installed, you can also perform OTA updates directly through the local **Web GUI**.

---

## Supported Hardware

- **Espressif:** ESP8266, ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6, WT32-ETH01
- **Raspberry Pi Pico:** RP2040, RP2350

## Supported LED Types

- **NeoPixel RGB:** WS2812b and compatible.
- **NeoPixel RGBW:** SK6812 (includes white channel calibration known from HyperSerial).
- **DotStar SPI:** APA102 and high-speed clocked LEDs.

## Integration

- **HyperHDR:** Native support via `udpraw` and `WLED` protocols. A dedicated DDP driver coming soon to HyperHDR.
- **Home Assistant:** Automatic discovery with support for power on/off, color, and brightness control.

## Network Services

| Service | Port | Protocol / Description |
| :--- | :--- | :--- |
| Web GUI | 80 | Device configuration |
| UDP DDP | 4048 | DDP listener |
| UDP RealTime | 21324 | Real-time stream listener |
| UDP Raw RGB | 5568 | Raw color stream listener |

---
*Developed for performance. Optimized for HyperHDR.*