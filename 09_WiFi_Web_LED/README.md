# Chapter 9.1 — WiFi Web Server LED Control

Ported from Freenove's `Sketch_09.1_WiFi_Web_Servers_LED.ino`. Connects
to WiFi (edit `ssid`/`password` at the top of `src/main.cpp` before
flashing) and serves a small HTML page with three colored cards; clicking
one sends an AJAX request that toggles the matching color channel on the
onboard WS2812 LED.
