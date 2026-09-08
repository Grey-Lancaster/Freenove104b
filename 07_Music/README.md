# Chapter 7.1 — Music (MP3 playback via ES8311 codec)

Ported from Freenove's `Sketch_07.1_Music.ino`. Plays `.mp3` files from the
root of an SD card (`SD_MMC`) through the onboard ES8311 codec.

## No-SD-card fallback

If no `.mp3` files are found on the SD card (or there's no card at all —
`SD_MMC.begin()` is tried once and moves on rather than retrying in a
loop, see below), the firmware falls back to a single track, **Olive.mp3**, baked directly
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

## Bug fixed (real cause of quiet audio): wrong MCLK multiple for 16-bit audio

`es8311.h` had `EXAMPLE_MCLK_MULTIPLE` set to **384**, with a comment
sitting right next to it saying *"if not using 24-bit data width, 256
should be enough"* — this project uses 16-bit
(`I2S_BITS_PER_SAMPLE_16BIT`), so 384 directly contradicted its own
comment. This value feeds `es8311_codec_init()`'s clock-coefficient
lookup, which programs the ES8311's internal DAC clock dividers over
I2C — a mismatched ratio there leaves the codec's DAC running with the
wrong internal filter/oversampling divider relative to what's actually
clocked into it once playback starts, which cuts the effective output
level substantially without showing up anywhere in software (volume
readouts, `Serial` logs, `audio.isRunning()` all looked completely
normal). `17_Lvgl_Music/src/es8311.h` already had the correct value
(256) and was noticeably louder as a result even at a much lower
`setVolume()`. Fixed by changing this chapter's value to match.

Confirmed on hardware after ruling out several other suspects that
turned out to be red herrings once actually tested side-by-side — worth
knowing about since they're real fixes, just not the cause of *this*
particular symptom:
- The `SD_MMC.begin()` retry loop below is still a legitimate fix (single
  attempt is simply more correct than retrying pointlessly with no card
  present), but it turned out the `Failed to initialize I2S bus!` message
  it was blamed for happens on *both* this chapter and `17_Lvgl_Music`
  regardless — a harmless artifact of a redundant manual pre-init call
  that the `Audio` library's own internal setup supersedes either way
  (confirmed via the periodic status line below, added specifically to
  get a reliable side-by-side comparison instead of guessing from a
  single chapter's logs in isolation).
- The digital volume fixes elsewhere in this README (bumping to 21,
  fixing `17_Lvgl_Music`'s slider) are real and worth keeping, but with
  the MCLK multiple wrong, no amount of digital gain could fully make up
  for the codec-level attenuation.

Freenove's original tutorial code also retries `SD_MMC.begin()` in a loop
(up to 10 attempts) before setting up the codec's I2S bus, which is worth
avoiding on its own merits even though it wasn't this bug's cause: with
no SD card inserted, every attempt fails, and on this chip SDMMC and I2S
draw from the same small shared pool of GDMA channels, so repeated failed
`SD_MMC.begin()` calls can leak/exhaust one for no benefit. Fixed by
calling `SD_MMC.begin()` once, matching `17_Lvgl_Music`'s
`driver_sdmmc.cpp`, which already gets this right.

## Periodic status line for debugging

This board's native USB CDC port drops and re-enumerates on every reset,
so any log tool (this project's own flasher included, and ESP Web
Tools/ESPHome's) that connects even slightly late after a reset misses
`setup()`'s one-time boot prints entirely and shows nothing — no actual
reset needed to reproduce this, just connecting a moment late is enough.
To sidestep that race for debugging, `loop()` prints a status line every
2 seconds — `status: i2s=<ok|FAILED> playing=<yes|no> vol=<n>` — so
connecting a serial monitor at any time, no reset timing required, still
shows current state within a couple seconds.

## Volume

Default volume is set to **21** (max, of 0–21) in `setup()`.
`setVolume()`'s argument isn't linear: it indexes a 22-entry lookup table
(`lib_freenove/ESP32-audioI2S/src/Audio.h`) that maps 0–21 to an internal
gain of `{0,1,2,3,4,6,8,10,12,14,17,20,23,27,30,34,38,43,48,52,58,64}` —
so 19 (the original default) was gain 52/64, not "19/21 of the way
there." Adjust `audio.setVolume(...)` in `src/main.cpp` if max is too
loud for your setup.
