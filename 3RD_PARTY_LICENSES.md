# Third-Party Licenses

This document contains a list of third-party software components, libraries, and frameworks used in the **Hyperk** application ecosystem, along with their respective licenses.

For the license governing the main open-source application code, please refer to the `LICENSE` file located in the root directory of the repository.

---

## Summary of Dependencies

| Component / Library | License | Author / Maintainer | Used in Environments |
| :--- | :--- | :--- | :--- |
| **Hyperk Backend (`libbackend.a`)** | Proprietary | awawa-dev | All |
| **HyperSerialEsp8266** | MIT License | awawa-dev | ESP8266 |
| **HyperSerialESP32** | MIT License | awawa-dev | ESP32 |
| **HyperSerialPico** | MIT License | awawa-dev | Raspberry Pi Pico (W) |
| **ESPAsyncWebServer** | Apache License 2.0 | ESP32Async / Me-No-Dev | All |
| **ArduinoJson** | MIT License | Benoit Blanchon | All |
| **Embedded Template Library (etl)** | MIT License | John Wellbelove | All |
| **esp-led-strip-async** (Fork) | Apache License 2.0 | awawa-dev / Espressif Systems | ESP32 (Modern-ESP) |
| **Pico.css** | MIT License | Pico.css Contributors | All |
| **Arduino Core for ESP32** | LGPL v2.1 | Espressif Systems / pioarduino | ESP32 |
| **Arduino Core for ESP8266** | LGPL v2.1 | ESP8266 Community | ESP8266 |
| **Arduino Core for RP2040/RP2350** | LGPL v2.1 | Earle F. Philhower, III | Raspberry Pi Pico (W) |

---

## Detailed License Information

### 1. Hyperk Backend (`libbackend.a`)

* **License:** Proprietary
* **Copyright:** Copyright (c) 2026 awawa-dev
* **Link:** https://github.com/awawa-dev/Hyperk-Backend

> **Notice:** The precompiled library `libbackend.a` contains proprietary software distributed in binary form only. Usage, redistribution, and integration are governed by the license provided with the Hyperk Backend distribution.

### 2. HyperSerialEsp8266

* **License:** MIT License
* **Copyright:** Copyright (c) 2026 awawa-dev
* **Link:** https://github.com/awawa-dev/HyperSerialEsp8266

### 3. HyperSerialESP32

* **License:** MIT License
* **Copyright:** Copyright (c) 2026 awawa-dev
* **Link:** https://github.com/awawa-dev/HyperSerialESP32

### 4. HyperSerialPico

* **License:** MIT License
* **Copyright:** Copyright (c) 2026 awawa-dev
* **Link:** https://github.com/awawa-dev/HyperSerialPico

### 5. ESPAsyncWebServer

* **License:** Apache License 2.0
* **Copyright:** Copyright (c) Me-No-Dev and contributors
* **Link:** https://github.com/ESP32Async/ESPAsyncWebServer

### 6. ArduinoJson

* **License:** MIT License
* **Copyright:** Copyright (c) Benoit Blanchon
* **Link:** https://github.com/bblanchon/ArduinoJson

### 7. Embedded Template Library (etl)

* **License:** MIT License
* **Copyright:** Copyright (c) John Wellbelove
* **Link:** https://github.com/ETLCPP/etl

### 8. esp-led-strip-async (Asynchronous Fork)

* **License:** Apache License 2.0
* **Copyright:** Copyright (c) Espressif Systems and contributors
* **Additional Modifications:** Copyright (c) 2026 awawa-dev
* **Link:** https://github.com/awawa-dev/esp-led-strip-async

> This component is based on Espressif's original implementation and contains modifications maintained by awawa-dev.

### 9. Pico.css

* **License:** MIT License
* **Copyright:** Copyright (c) Pico.css Contributors
* **Link:** https://github.com/picocss/pico

### 10. Arduino Frameworks & Toolchains

* **License:** GNU Lesser General Public License v2.1 (LGPL v2.1) and Apache License 2.0 (ESP-IDF components)
* **Copyright:** Respective authors and contributors
* **Links:**
  * **ESP32 Core:** https://github.com/espressif/arduino-esp32
  * **ESP32 PlatformIO Fork:** https://github.com/pioarduino/platform-espressif32
  * **ESP8266 Core:** https://github.com/esp8266/Arduino
  * **RP2040/RP2350 Core:** https://github.com/earlephilhower/arduino-pico

---

## License Notices

The software components listed above are distributed under their respective licenses.

Copies of the applicable license texts should be included with Hyperk distributions where required by the original license terms.

---

## MIT License (Summary)

Permission is hereby granted, free of charge, to any person obtaining a copy of the software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

---

## Apache License 2.0 (Summary)

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this software except in compliance with the License.

A copy of the License may be obtained at:

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.