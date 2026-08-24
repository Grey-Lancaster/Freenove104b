# XiaoZhi AI Voice Assistant — Freenove FNK0104B

Stock XiaoZhi AI voice assistant running on Grey's Freenove FNK0104B (2.8" ILI9341 display) board.

## Status

Working end-to-end: boots clean, connects to WiFi, voice wake word ("Hi, ESP") and BOOT-button push-to-talk both trigger listening, gets replies. Agent name in xiaozhi.me console: **Papa Lanc**.

Wake word uses the `wn9s_hiesp` model (not the original `wn9_hiesp`) — confirmed via live testing to detect noticeably faster (~16s vs ~28s in back-to-back tests) with clean multi-turn conversations following.

Freenove's prebuilt binaries (both the doc's `Upload_Xiaozhi_Bin` folder and their web flasher) crash-loop on this board and don't work — fixed by building from source instead. Root cause of the prebuilt failure remains unresolved (see project memory / commit history); it isn't a PSRAM config issue, that setting is identical and correct in both.

## What's in this repo

This repo holds Grey's own build artifacts and config for this board — not a vendored copy of Freenove's source (that lives at [github.com/Freenove/xiaozhi-esp32](https://github.com/Freenove/xiaozhi-esp32), forked from [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)).

- `firmware/` — the built binaries flashed to the board (`bootloader.bin`, `partition-table.bin`, `ota_data_initial.bin`, `srmodels.bin`, `xiaozhi.bin`, `merged-binary.bin`)
- `build-config/sdkconfig` — the exact resolved config used, with two fixes applied on top of Freenove's plain defaults:
  - `CONFIG_SR_WN_WN9S_HIESP=y` (wake word — plain default is the Mandarin `NIHAOXIAOZHI` model, not "Hi, ESP"; the "S" variant is a newer/smaller model that tested faster than the plain `WN9_HIESP`)
  - `CONFIG_LANGUAGE_EN_US=y` (system UI language — plain default is `ZH_CN`, causes Chinese notification/status text)
- `custom-source/` — Grey's own code changes on top of Freenove's fork (not vendored upstream source). Currently just the push-OTA server — see its own README for what it does and how to reapply it to a fresh checkout.

## Updating the firmware over the network (no BOOT button needed)

As of the push-OTA server (see `custom-source/`), updates after the initial flash don't need physical access to the board:

```bash
curl -X POST -H "X-OTA-Token: REDACTED-TOKEN" --data-binary @firmware/xiaozhi.bin http://<device-ip>:8080/update
```

Confirmed working end-to-end 2026-08-24. Only updates app code — a wake-word model change still needs a full manual flash.

From-source build environment on this machine lives at `C:\esp-idf` (ESP-IDF v5.4.2) and `C:\dev\xiaozhi-esp32-src` (Freenove's fork, already configured to match `build-config/sdkconfig` above).

## To-Do

- [ ] **Connect XiaoZhi to Home Assistant via MCP**, so voice commands can control lights and query sensors (e.g. temperature). Steps (needs to happen while home, on the same network as Home Assistant):
  1. In Home Assistant: Settings → Devices & Services → Add Integration → **"Model Context Protocol Server"**. Configure which entities it exposes (lights, temperature sensors, etc.).
  2. Install [HACS](https://hacs.xyz/) if not already installed.
  3. In HACS → Integrations → ⋮ → Custom repositories → add `https://github.com/mac8005/xiaozhi-mcp-ha` as type "Integration". Install "Xiaozhi MCP", restart Home Assistant.
  4. Create a Home Assistant long-lived access token: Settings → People → your user → Long-lived access tokens.
  5. Get the XiaoZhi MCP endpoint from the xiaozhi.me console (under the Papa Lanc agent's settings — exact menu location not yet confirmed, find together next time).
  6. In Home Assistant: Settings → Devices & Services → Add Integration → **"Xiaozhi MCP"** → enter name, xiaozhi.me endpoint URL, HA access token, scan interval (30s default).
