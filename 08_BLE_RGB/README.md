# Chapter 8.2 — BLE-Controlled RGB LED

Ported from Freenove's `Sketch_08.2_BLE_RGB.ino`. Same BLE UART bridge as
chapter 8.1, but incoming text commands (`red_on`/`red_off`,
`green_on`/`green_off`, `blue_on`/`blue_off`) drive the onboard WS2812
LED's color instead of just being echoed back.
