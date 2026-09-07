# Chapter 7.1 — Music (MP3 playback via ES8311 codec)

Ported from Freenove's `Sketch_07.1_Music.ino`. Plays `.mp3` files from the
root of an SD card (`SD_MMC`) through the onboard ES8311 codec.

## No-SD-card fallback

If no `.mp3` files are found on the SD card (or there's no card at all —
`SD_MMC.begin()` is tried once and moves on rather than hanging forever,
see the I2S bug below for why it's *only* tried once), the firmware falls
back to a single track, **Olive.mp3**, baked directly
into the binary (`src/olive_mp3.h`, generated via `xxd -i`, ~327KB) and
written to SPIFFS on first boot. Playback then reads from SPIFFS instead
of the SD card. This means the demo always plays *something* out of the
box, even with no SD card inserted.

## Bug fixed: silent speaker (missing MCLK)

Freenove's original tutorial code initializes the audio library as:

```cpp
audio.setPinout(I2S_BCK, I2S_WS, I2S_DOUT, I2S_MCK);
```

`Audio::setPinout()`'s actual signature is
`setPinout(BCLK, LRC, DOUT, DIN = I2S_PIN_NO_CHANGE, MCK = I2S_PIN_NO_CHANGE)`
— the 4th parameter is **DIN** (mic input), not MCK. So `I2S_MCK` was
landing in the wrong argument slot, and the real MCK parameter was left
at its default of "don't touch this pin." The codec's I2C register init
still reports success and the mic still works either way, so this is
easy to miss — but with no real MCLK signal on GPIO4, the speaker stays
silent. Fixed here by passing `I2S_PIN_NO_CHANGE` for DIN and `I2S_MCK`
explicitly as the 5th argument. (DIN isn't wired up via this library in
this project, so leaving it unset is correct.)

The sibling `translate` project ([github.com/Grey-Lancaster/translate](https://github.com/Grey-Lancaster/translate))
independently hit and documented this same MCLK requirement for this
exact ES8311/board combo.

## Bug fixed: quiet/degraded audio from a failed I2S init

Freenove's original tutorial code retries `SD_MMC.begin()` in a loop (up
to 10 attempts) before setting up the codec's I2S bus. With no SD card
inserted, every attempt fails — and on this chip, SDMMC and I2S draw from
the same small shared pool of GDMA channels, so the repeated failed
`SD_MMC.begin()` calls leak/exhaust one. By the time `driver_es8311_init()`
calls `i2s_driver_install()` afterward, there's nothing left for it, and
it fails outright (`Failed to initialize I2S bus!` in the serial log) —
with no SD card involved in that failure at all. Whatever audio comes out
afterward is running through a bus that never initialized correctly,
which is why this chapter sounded noticeably quieter/weaker than
`17_Lvgl_Music` even at a higher `setVolume()` value, despite both using
identical codec/volume settings. Fixed by calling `SD_MMC.begin()` once,
matching `17_Lvgl_Music`'s `driver_sdmmc.cpp`, which already gets this
right.

## Volume

Default volume is set to **21** (max, of 0–21) in `setup()`.
`setVolume()`'s argument isn't linear: it indexes a 22-entry lookup table
(`lib_freenove/ESP32-audioI2S/src/Audio.h`) that maps 0–21 to an internal
gain of `{0,1,2,3,4,6,8,10,12,14,17,20,23,27,30,34,38,43,48,52,58,64}` —
so 19 (the original default) was gain 52/64, not "19/21 of the way
there." Adjust `audio.setVolume(...)` in `src/main.cpp` if max is too
loud for your setup.
