# Chapter 17.1 — LVGL Music Player

Ported from Freenove's `Sketch_17.1_Lvgl_Music.ino`. An LVGL touchscreen UI
(play/pause/stop/prev/next, volume slider, progress bar) for `.mp3` files
on the root of an SD card's `/music` folder, played through the onboard
ES8311 codec.

## No-SD-card fallback

Mirrors the fallback added in `07_Music`: if the SD card's `/music` folder
has no files (or there's no card at all), the firmware writes an embedded
track, **Olive.mp3** (`src/olive_mp3.h`, ~327KB, same asset as `07_Music`),
to SPIFFS on first boot and plays it from there instead. Unlike
`07_Music`'s button-driven original, this fallback **autoplays
immediately** on load — the on-screen label already reads "No files found
on SD card - playing local file", so it starts playing rather than sitting
idle waiting for a press of the play button. The left/right/play controls
work fine on the single fallback track too (they just keep replaying it).

New helpers in `src/music_ui.cpp` — `music_track_count()`,
`music_track_name()`, `music_track_path()` — abstract over "SD card
listing" vs. "single SPIFFS fallback track" so the existing button
handlers didn't need separate fallback-aware branches.

## Bug fixed: silent speaker (missing MCLK)

Same root cause as `07_Music` (see that chapter's README for the full
explanation): `music_iis_init()` called

```cpp
audio.setPinout(AUDIO_I2S_BCK, AUDIO_I2S_WS, AUDIO_I2S_DOUT, AUDIO_I2S_MCK);
```

which put the MCLK pin into `setPinout()`'s 4th parameter (`DIN`, mic
input) instead of the actual 5th `MCK` parameter — so the codec never got
a real MCLK signal on GPIO4 and stayed silent even though init reported
success. Fixed by passing `I2S_PIN_NO_CHANGE` for DIN and `AUDIO_I2S_MCK`
explicitly as the 5th argument. Confirmed against the sibling `translate`
project ([github.com/Grey-Lancaster/translate](https://github.com/Grey-Lancaster/translate)),
which independently documented this same MCLK requirement for this exact
codec/board.
