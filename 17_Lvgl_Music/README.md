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

## Bug fixed: startup volume ignored the slider

The volume slider was created with:

```cpp
lv_slider_set_range(ui->music_slider_valume, 0, 21);
lv_slider_set_value(ui->music_slider_valume, 10, LV_ANIM_OFF);
```

`lv_slider_set_value()` only moves the widget — it does **not** fire
LVGL's value-changed event, which is what the slider's own handler
(`music_slider_change_event_handler`) relies on to actually call
`music_set_volume()` → `audio.setVolume()`. So at startup the slider
*displayed* 10 but `audio.setVolume()` was never called at all, leaving
the codec at `Audio`'s uninitialized class-member default of `m_vol = 64`
— i.e. **max volume**, louder than even an explicit `setVolume(21)` —
until the user actually dragged the slider once. Fixed by calling
`music_set_volume(10)` immediately after the `lv_slider_set_value()` call
so playback actually starts at what's displayed. (This chapter also
sounded louder than `07_Music` even after that fix — but that turned out
to be a separate, larger bug in `07_Music` itself, a wrong codec MCLK
multiple; see that chapter's README.)

## Periodic status line for debugging

Same addition as `07_Music`, for the same reason: this board's native USB
CDC port drops and re-enumerates on every reset, so any log tool that
connects even slightly late after a reset misses `setup()`'s one-time
boot prints entirely and shows nothing — no actual reset needed to
reproduce this, just connecting a moment late is enough. `loop()` prints
`status: i2s=<ok|FAILED> playing=<yes|no> vol=<n>` every 2 seconds so
connecting at any time still shows current state within a couple
seconds. This is specifically what let us confirm, side-by-side with
`07_Music`, that `i2s=FAILED` happens on both chapters and isn't actually
the differentiator it initially looked like.
