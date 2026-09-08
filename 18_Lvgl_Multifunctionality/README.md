# Chapter 18.1 — LVGL Multifunctionality

Ported from Freenove's `Sketch_18.1_Lvgl_Multifunctionality.ino`. Combines
the picture viewer, chronograph, WS2812 color picker, and music player
screens from the earlier LVGL chapters into one app with a home/launcher
screen to switch between them. Includes its own copy of the SD card and
ES8311 audio codec init (for the music screen) alongside the LVGL/display
setup used by the other screens.

## Fixes ported from `07_Music`/`17_Lvgl_Music`

This chapter's `music_ui.cpp` was forked from Freenove's original code
before those two chapters' audio fixes existed, so it had the same bugs:
`music_iis_init()`'s `audio.setPinout()` call was missing the real MCLK
argument (landing `AUDIO_I2S_MCK` in the `DIN` slot instead, leaving the
codec without a real MCLK signal and the speaker silent), and the volume
slider's `lv_slider_set_value(..., 10, ...)` never actually called
`audio.setVolume()` (no value-changed event fires from a programmatic
`set_value`), leaving the codec at `Audio`'s uninitialized default of max
volume regardless of what the slider displayed. Both fixed the same way
as in the other two chapters — see their READMEs for the full
explanation. (`EXAMPLE_MCLK_MULTIPLE` in this chapter's `es8311.h` was
already the correct value, 256, unlike `07_Music`'s bug.)

## No-SD-card fallbacks

- **Music screen:** if the SD card's `/music` folder has no files (or
  there's no card at all), falls back to the same embedded **Olive.mp3**
  used by `07_Music`/`17_Lvgl_Music` (`src/olive_mp3.h`), written to
  SPIFFS and autoplayed — same pattern as those two chapters.
- **Picture viewer:** if the SD card's `/picture` folder has no files (or
  there's no card at all), shows an embedded fallback image instead of a
  blank screen — "The Grey Fox" logo, converted to a 240x240 raw RGB565
  C array (`src/fallback_img.cpp`/`.h`) matching `lv_img.cpp`'s own image
  format (`LV_COLOR_DEPTH==16`, `LV_COLOR_16_SWAP==0`: 2 bytes/pixel,
  low byte first, no alpha channel — verified against `lv_img.cpp`'s
  existing icon arrays before generating this one, since getting that
  byte layout wrong would show corrupted/garbled colors instead of
  failing to compile). This also fixed a pre-existing bug in
  `picture_imgbtn_display()`: the "no files" branch checked `name !=
  NULL`, but `get_file_name_by_index()` always returns an empty `String`
  (never `NULL`) when there's nothing to show, so that branch was
  actually dead code — the screen just went blank instead. Now checks
  for an empty string too.
