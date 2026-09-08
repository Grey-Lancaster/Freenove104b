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
