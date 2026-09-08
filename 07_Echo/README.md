# Chapter 7.2 — Mic Record + Playback (Echo)

Ported from Freenove's `Sketch_07.2_Echo.ino`. On a repeating loop:
records 5 seconds of audio from the ES8311 codec's mic input into a
PSRAM buffer, then plays that recording back out through the speaker.

Freenove's original code used the newer `ESP_I2S.h`/`I2SClass` API
(`recordWAV()`/`playWAV()`), which isn't present in this project's
pinned `arduino-esp32` core — ported to the older `I2S.h` global `I2S`
object instead, including a manual MCLK pin reach-through since
`I2SClass::begin()` has no MCLK parameter of its own.

This chapter never actually worked out of the box — three real bugs,
found and fixed together:

## Bug fixed: mic gain never set

`es8311.cpp` had `es8311_microphone_gain_set(...)` commented out — this
is a separate digital gain stage from the PGA gain
`es8311_microphone_config()` already sets, and leaving it unset made
recordings essentially inaudible. The sibling `translate` project
(github.com/Grey-Lancaster/translate) hit and documented this exact
issue on the same codec: even 24dB wasn't enough for a quieter voice,
needed the max, 42dB. Fixed by enabling it at 42dB.

## Bug fixed: playback completing instantly / static

Plain `I2S.write()` resolves to `write_nonblocking()`, which **silently
drops** (not queues) anything that doesn't fit in the small (~8KB)
output ring buffer right now, returning immediately either way — that's
why playback used to complete almost instantly and produce static, not
audio: most of each chunk was thrown away, never reaching the codec.
Switched to `I2S.write_blocking()`, which genuinely blocks until each
chunk is queued. Also mutes the speaker amp (`AP_ENABLE`) except while
actually playing, since the DAC output settles to audible static/noise
once playback stops otherwise.

## Bug fixed (in the framework itself): recordings captured at half rate

Even after the above, a "5 second" recording took ~10 real seconds, and
played back sped up/compressed rather than at normal pitch — waiting
longer didn't fix it, so this wasn't just slow delivery, it was genuine
data loss. Root cause, found by reading the bundled Arduino-ESP32 `I2S`
library's source directly: `I2SClass::_rx_done_routine()` (in
`libraries/I2S/src/I2S.cpp`) is missing the `*2`-for-stereo factor its
TX counterpart `_tx_done_routine()` has on the otherwise-identical line,
so it only drains half of each stereo DMA transfer into the ring buffer
per RX event — the other half gets overwritten before anything comes
back for it. This can't be fixed from sketch code (it's in the vendored
framework package, not this project), so `lib/I2S/` here is a
project-local copy of that library with the one-line fix applied —
PlatformIO's library resolution prefers a project's own `lib/` folder
over the framework's bundled one, so this doesn't touch the shared
PlatformIO package install and stays fully git-tracked/portable. Filed
upstream: [espressif/arduino-esp32#12892](https://github.com/espressif/arduino-esp32/issues/12892).
Also switched recording to stop by real elapsed time (`RECORD_SECONDS`)
rather than a fixed nominal byte count, and play back exactly however
many bytes actually got captured, so the demo's actual behavior always
matches whatever the current I2S throughput really is.
