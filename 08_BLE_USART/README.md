# Chapter 8.1 — BLE UART Bridge

Ported from Freenove's `Sketch_08.1_BLE_USART.ino`. Exposes a
Nordic-UART-Service-style BLE characteristic pair: anything typed into
the Serial monitor gets sent to a connected BLE client, and anything the
client writes gets echoed back over serial.
