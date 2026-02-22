# Privacy & Technical Note

This note applies to the web flasher and the OTA interface served by supported hardware devices (ESP32, ESP8266, Pico, etc.).

## 1. Cloudflare Workers & GitHub

To bypass CORS and ensure fast access to firmware, requests are routed through **Cloudflare Workers** (proxy + cache).

* Your IP address and technical headers are processed by Cloudflare (our data processor) to fetch data from GitHub.
* We do not store or log this data on our end.

## 2. Privacy-First Analytics

We use **Cloudflare Web Analytics** to monitor performance. It is a privacy-first tool that does not use cookies and does not track individual users.

• [Cloudflare Privacy Policy](https://www.cloudflare.com/privacypolicy/)
• [GitHub Privacy Statement](https://docs.github.com/en/site-policy/privacy-policies/github-privacy-statement)

Processing is based on legitimate interest (Art. 6(1)(f) GDPR) to deliver update functionality.  
Last updated: February 2026