# Chapter 7.2 — Mic Record + Playback (Echo)

Ported from Freenove's `Sketch_07.2_Echo.ino`. On a repeating loop:
records 5 seconds of audio from the ES8311 codec's mic input into a
PSRAM buffer, then plays that recording back out through the speaker.

Freenove's original code used the newer `ESP_I2S.h`/`I2SClass` API
(`recordWAV()`/`playWAV()`), which isn't present in this project's
pinned `arduino-esp32` core — ported to the older `I2S.h` global `I2S`
object instead, including a manual MCLK pin reach-through since
`I2SClass::begin()` has no MCLK parameter of its own.
