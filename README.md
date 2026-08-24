# FNK0104B Projects

Firmware projects and build artifacts for Grey's Freenove FNK0104B (ESP32-S3, 2.8" display) board. Each subfolder is a separate project.

## Projects

- [`xiaozhi/`](xiaozhi/) — stock XiaoZhi AI voice assistant

## Note on PSRAM

This board's ESP32-S3-WROOM-1 module uses octal PSRAM (N16R8: 16MB flash, 8MB octal PSRAM). Some prebuilt firmware for this board has failed to boot with an instant, silent crash loop on this chip — that turned out *not* to be a PSRAM configuration problem (the octal-PSRAM settings were already correct), but PSRAM/flash timing is worth ruling out early if a new project on this board hits a similar boot loop. See the relevant project's own README for specifics.
