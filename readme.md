# Hyperk

Hyperk is a minimalist, high-performance uni-platform WiFi LED driver for ESP8266, ESP32 (S2, S3, C2, C3, C5, C6), Raspberry Pi Pico W (RP2040, RP2350). Designed as a lightweight and streamlined component that avoids unnecessary complexity, it delivers low‑latency performance and integrates smoothly with platforms such as HyperHDR, while offering essential home‑automation capabilities through a clean, modern codebase.

## Installation

The firmware can be flashed directly from your browser:
**[hyperk.hyperhdr.org](https://hyperk.hyperhdr.org)**

> [!TIP]
> Once installed, you can also perform OTA updates directly through the local **Web GUI**.

---

## Supported Hardware

- **Espressif:** ESP8266, ESP32, ESP32-S2, ESP32-S3, ESP32-C2, ESP32-C3, ESP32-C5, ESP32-C6, WT32-ETH01
- **Raspberry Pi Pico:** RP2040, RP2350

## Supported LED Types

- **NeoPixel RGB:** WS2812b and compatible.
- **NeoPixel RGBW:** SK6812 (includes white channel calibration known from HyperSerial).
- **DotStar SPI:** APA102 and high-speed clocked LEDs.

## Manual

👉 [https://wiki.hyperhdr.eu/Hyperk](https://wiki.hyperhdr.eu/Hyperk)

## Integration

- **HyperHDR:** Dedicated Hyperk driver (>v22beta1). Also works via its native `DDP`, `udpraw`, and `WLED` drivers.
- **Home Assistant:** Automatic discovery with support for power on/off, color, and brightness control.

|      HyperHDR      |   Home Assistant   |
|--------------------|--------------------|
| [![1](https://github.com/user-attachments/assets/ab252845-50da-4985-96be-d1da7bfd522d)](https://github.com/user-attachments/assets/ab252845-50da-4985-96be-d1da7bfd522d) | [![2a](https://github.com/user-attachments/assets/b3097b11-249a-4a66-9cff-bcdd70f28e87)](https://github.com/user-attachments/assets/b3097b11-249a-4a66-9cff-bcdd70f28e87) |


## Network Services

| Service | Port | Protocol / Description |
| :--- | :--- | :--- |
| Web GUI | 80 | Device configuration |
| UDP DDP | 4048 | DDP listener |
| UDP RealTime | 21324 | Real-time stream listener |
| UDP Raw RGB | 5568 | Raw color stream listener |
  
<small>LEDs turn off automatically 6.5s after stream loss.</small>

---
*Developed for performance. Optimized for HyperHDR. [Privacy & Technical Note](https://awawa-dev.github.io/hyperk/privacy.html)*
