# FNK0104B Projects

Firmware projects and build artifacts for Grey's Freenove FNK0104B (ESP32-S3, 2.8" display) board. Each subfolder is a separate project.

## Projects

- [`xiaozhi/`](xiaozhi/) — stock XiaoZhi AI voice assistant
- `01_SerialRW/` through `19_LVGL_Arduino/` — Freenove's official FNK0104B
  "Touch Tutorial" (19 chapters), ported from Arduino IDE to PlatformIO. See
  below.

## Touch Tutorial (chapters 1–19)

Freenove's own tutorial ([docs.freenove.com/projects/fnk0104](https://docs.freenove.com/projects/fnk0104/en/latest/fnk0104/codes/MAIN.html))
ships as Arduino IDE sketches; each chapter here is the same code ported to
build with `pio run` instead, fixing several compatibility gaps between
Freenove's own bundled libraries/tutorial code and the arduino-esp32 core
version this board's PSRAM/TFT_eSPI setup depends on (legacy vs. new I2S
driver APIs, a missing board-variant define, an unassigned ES8311 handle,
etc.) — see each chapter's source comments for specifics.

| Chapter | Folder | Covers |
|---|---|---|
| 1 | `01_SerialRW/` | Serial read/write |
| 2.1 | `02_LedPixel/` | Onboard WS2812 RGB LED |
| 2.2 | `02_Rainbow/` | WS2812 color wheel animation |
| 3.1 | `03_Button_RGB/` | Button input + RGB LED |
| 4.1 | `04_Button_Interrupt_UART/` | Button interrupt + debounce |
| 5.1 | `05_Battery_Voltage/` | Battery ADC reading |
| 6.1 | `06_SDMMC_Test/` | SD card (SD_MMC) file I/O |
| 7.1 | `07_Music/` | MP3 playback via ES8311 codec |
| 7.2 | `07_Echo/` | Mic record + playback loopback |
| 8.1 | `08_BLE_USART/` | BLE UART bridge |
| 8.2 | `08_BLE_RGB/` | BLE-controlled RGB LED |
| 9.1 | `09_WiFi_Web_LED/` | WiFi web server LED control |
| 10.1 | `10_TFT_Rainbow/` | Raw TFT_eSPI drawing/fonts |
| 10.2 | `10_Flash_Jpg_DMA/` | JPEG decode from flash to TFT |
| 11.1 | `11_Touch/` | FT6336U capacitive touch (raw) |
| 12.1 | `12_TFT_Touch_Draw/` | Touch-driven drawing on TFT |
| 13.1 | `13_LVGL/` | Baseline LVGL setup |
| 14.1 | `14_Lvgl_Picture/` | LVGL image viewer from SD card |
| 15.1 | `15_Lvgl_Timer/` | LVGL chronograph/stopwatch UI |
| 16.1 | `16_Lvgl_WS2812/` | LVGL-driven WS2812 color picker |
| 17.1 | `17_Lvgl_Music/` | LVGL music player UI |
| 17.2 | `17_Lvgl_Echo/` | LVGL mic record + playback (unofficial — not one of Freenove's 19, pairs with 07_Echo the way 17_Lvgl_Music pairs with 07_Music) |
| 18.1 | `18_Lvgl_Multifunctionality/` | All LVGL screens combined into one app |
| 19.1 | `19_LVGL_Arduino/` | Stock LVGL widgets demo |

Shared across all 19 chapters:
- `boards/ESP32-S3-WROOM-1-N16R8.json` — custom board def (the generic
  esp32-s3-devkitc-1 def has no PSRAM and crashes on boot for this module).
- `common.ini` — shared PlatformIO environments (`base` → `tft_base` →
  `lvgl_base`) with the known-good TFT_eSPI/touch/LVGL config, extended by
  each chapter's own `platformio.ini`.
- `include/lv_conf.h` — shared LVGL configuration.
- `lib_freenove/` — Freenove's own Arduino libraries (WS2812, FT6336U touch,
  TJpg_Decoder, LVGL, ESP32-audioI2S), vendored and in some cases patched for
  compatibility with this project's pinned core version.

Build any chapter with:
```
cd 01_SerialRW
pio run
```

## Note on PSRAM

This board's ESP32-S3-WROOM-1 module uses octal PSRAM (N16R8: 16MB flash, 8MB octal PSRAM). Some prebuilt firmware for this board has failed to boot with an instant, silent crash loop on this chip — that turned out *not* to be a PSRAM configuration problem (the octal-PSRAM settings were already correct), but PSRAM/flash timing is worth ruling out early if a new project on this board hits a similar boot loop.

One concrete case: this board's octal PSRAM requires **DIO** flash mode specifically (PlatformIO's own build already bakes the right mode into each chapter's `bootloader.bin`/`firmware.bin` image headers). The `docs/` web flasher merges those files into one flat image per chapter with `esptool merge-bin`, and merge-bin will happily overwrite that correct mode if you pass an explicit `--flash-mode` (e.g. `qio`) instead of `--flash-mode keep`. Forcing QIO mode makes the ROM's very first flash read after `ets_loader.c` unreliable, so the board watchdog-resets before printing anything — no bootloader banner, no app output, nothing — which looks exactly like a PSRAM crash but is actually just a wrong flash-mode byte in the merged image. Always merge with `--flash-mode keep --flash-freq keep --flash-size keep`, and verify with `esptool image-info <merged-bin>` that it still reports `Flash mode: DIO` before shipping a new chapter's `.bin`.
