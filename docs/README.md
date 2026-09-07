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
  --flash-mode qio --flash-freq 80m --flash-size 16MB \
  0x0     .pio/build/esp32s3/bootloader.bin \
  0x8000  .pio/build/esp32s3/partitions.bin \
  0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
  0x10000 .pio/build/esp32s3/firmware.bin
```

`boot_app0.bin` is identical for every chapter (same Arduino core version) — only the other three files change per chapter. After adding a new chapter's `.bin`, remove its `disabled` attribute and `data-bin="firmware/<chapter>.bin"` on the matching `<option>` in `index.html`.

Built so far: `01_SerialRW`, `07_Music`, `17_Lvgl_Music`. 20 chapters remain.
