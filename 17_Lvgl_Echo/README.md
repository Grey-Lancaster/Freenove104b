# Chapter 17.2 — LVGL Mic Record + Playback (unofficial)

Not one of Freenove's own 19 tutorial chapters — a new addition pairing
`07_Echo`'s mic record/playback demo with an LVGL touchscreen UI, the way
`17_Lvgl_Music` pairs with `07_Music`.

Two buttons and a status label: **Record (5s)** records from the ES8311's
mic input into a PSRAM buffer (allocated once at startup, reused for every
take), showing "Recording... Ns" while it runs; **Play** (disabled until a
recording exists) plays that buffer back out through the speaker, showing
"Playing... Ns". Both run on a background FreeRTOS task (same convention
`17_Lvgl_Music`'s `loopTask_music` uses) so the multi-second blocking I2S
read/write doesn't freeze the screen — LVGL keeps redrawing the status
label live while either one is in progress.

Reuses `07_Echo`'s exact I2S setup (`I2S.h`'s legacy global `I2S` object,
not the newer `ESP_I2S.h`/`I2SClass` Freenove's original sketch used and
isn't present in this project's pinned `arduino-esp32` core) and codec
driver (`es8311.cpp`/`.h`/`es8311_reg.h`, copied unchanged — already had
the correct `EXAMPLE_MCLK_MULTIPLE` of 256).

## Bugs fixed

This was copied from `07_Echo` before that chapter's own bugs were
found and fixed, so it needed the exact same three fixes — full
writeup in [`07_Echo/README.md`](../07_Echo/README.md):

1. **Mic gain never set** (`es8311.cpp`) — recordings were essentially
   inaudible until `es8311_microphone_gain_set()` was enabled at 42dB.
2. **Playback completing instantly / static** — plain `I2S.write()`
   silently drops anything that doesn't fit the output ring buffer;
   switched to `I2S.write_blocking()`, and the speaker amp
   (`AP_ENABLE`) is now muted except while actually playing.
3. **Recordings captured at half rate, playing back sped up** — a real
   bug in the bundled Arduino-ESP32 `I2S` library itself: its receive
   path drains only half of each stereo DMA transfer per event (missing
   a `*2`-for-stereo factor the transmit path has). Filed upstream as
   [espressif/arduino-esp32#12892](https://github.com/espressif/arduino-esp32/issues/12892).
   `lib/I2S/` here is a project-local copy of that library with the
   one-line fix applied (PlatformIO prefers a project's own `lib/`
   folder over the framework's bundled one, so this is fully
   git-tracked and doesn't touch the shared PlatformIO package install).
   Recording also now stops by real elapsed time rather than a fixed
   nominal byte count, and plays back exactly however many bytes
   actually got captured.
