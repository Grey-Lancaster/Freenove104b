# Freenove104b Touch Tutorial Flasher

Browser-based flasher (Web Serial + [esptool-js](https://github.com/espressif/esptool-js), no install) for the 19-chapter Touch Tutorial in this repo. Live at https://grey-lancaster.github.io/Freenove104b/.

Pick a chapter from the dropdown, connect the board, flash. Each chapter is a standalone demo, so it's always a single full-flash write at `0x0` — no OTA/app-update mode.

## Building a chapter's firmware/*.bin

Each `firmware/<chapter>.bin` is a merged flat image built like this:

```
cd <chapter folder>
pio run

python -m esptool --chip esp32s3 merge-bin \
  -o ../docs/firmware/<chapter>.bin \
  --flash-mode keep --flash-freq keep --flash-size keep \
  0x0     .pio/build/esp32s3/bootloader.bin \
  0x8000  .pio/build/esp32s3/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/esp32s3/firmware.bin
```

`boot_app0.bin` is identical for every chapter (same Arduino core version) — only the other three files change per chapter. After adding a new chapter's `.bin`, remove its `disabled` attribute and `data-bin="firmware/<chapter>.bin"` on the matching `<option>` in `index.html`.

**Always use `keep`/`keep`/`keep`, never hardcode `--flash-mode`.** This board's octal PSRAM shares pins with the flash bus in a way that requires **DIO** flash mode specifically — pio's own build already bakes the right mode into `bootloader.bin`/`firmware.bin`'s image headers, but `merge-bin` will happily overwrite that if you pass explicit `--flash-mode qio` (or anything other than `keep`). The result boots into an instant, silent watchdog-reset loop before any of our own code (or even the second-stage bootloader's own log line) prints anything, because the ROM's first flash read after `ets_loader.c` already fails. Verify with `esptool image-info <merged-bin>` and confirm `Flash mode: DIO` before shipping a new chapter's `.bin`.

Built so far: `01_SerialRW`, `07_Music`, `17_Lvgl_Music`, `18_Lvgl_Multifunctionality`. 19 chapters remain.

## Testing a just-pushed firmware fix

GitHub Pages sends `Cache-Control: max-age=600` on files under `firmware/`. `app.js` fetches with `cache: "no-store"` specifically so re-testing a chapter you already picked in the last 10 minutes doesn't silently serve the stale `.bin` from your own browser cache instead of the fix you just pushed — if you ever see old behavior right after a redeploy, hard-refresh the page (or check with `curl -sI .../firmware/<chapter>.bin` for `X-Cache: HIT` vs the file's actual hash) before assuming the fix didn't work.
