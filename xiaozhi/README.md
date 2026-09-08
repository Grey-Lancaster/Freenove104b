# XiaoZhi AI Voice Assistant — Freenove FNK0104B

Stock XiaoZhi AI voice assistant running on Grey's Freenove FNK0104B (2.8" ILI9341 display) board.

## Status

Working end-to-end: boots clean, connects to WiFi, voice wake word ("Hi, ESP") and BOOT-button push-to-talk both trigger listening, gets replies. Agent name in xiaozhi.me console: **Papa Lanc**.

Wake word uses the `wn9s_hiesp` model (not the original `wn9_hiesp`) — confirmed via live testing to detect noticeably faster (~16s vs ~28s in back-to-back tests) with clean multi-turn conversations following.

Freenove's prebuilt binaries (both the doc's `Upload_Xiaozhi_Bin` folder and their web flasher) crash-loop on this board and don't work — fixed by building from source instead. Root cause of the prebuilt failure remains unresolved (see project memory / commit history); it isn't a PSRAM config issue, that setting is identical and correct in both.

## What's in this repo

This repo holds Grey's own build config for this board — not a vendored copy of Freenove's source (that lives at [github.com/Freenove/xiaozhi-esp32](https://github.com/Freenove/xiaozhi-esp32), forked from [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)).

**The built binaries are deliberately *not* committed here** (removed and purged from history 2026-09-08, see `.gitignore`). Two separate reasons, worth understanding since they have different implications:

- **Device identity is hardware-bound, not binary-bound.** XiaoZhi's activation (`main/ota.cc` in [78/xiaozhi-esp32](https://github.com/78/xiaozhi-esp32)) derives the device's identity from the chip's own eFuse user-data block and MAC address, not from anything in the compiled firmware. So flashing `merged-binary.bin` onto *the same physical dev board* used to build/activate it reconnects to Grey's existing "Papa Lanc" agent on xiaozhi.me (confirmed — that's what happened testing the web flasher). Flashing the *identical file* onto a **different** board would give it a different MAC/eFuse identity and require its own fresh xiaozhi.me activation — it would not inherit Papa Lanc or any account access. In other words: the risk isn't really about the file's content, it's specific to this one board.
- **The OTA push token is a real, binary-content secret**, independent of hardware: `OTA_PUSH_TOKEN` in `custom-source/main/ota_push_server.cc` is a plain string comparison, so anyone on the same LAN as *any* board running this exact build (regardless of whose board it is) could use it to push arbitrary firmware. This one *does* travel with the file.

Plan: leave `firmware/` out of the repo until a second, different physical board can be tested end-to-end (confirming the "different board = different identity" reasoning above actually holds) — then reconsider whether to commit the binaries again. Rebuild locally into `firmware/` (gitignored) from `xiaozhi-esp32-src` in the meantime; see below for where that source lives.

- `build-config/sdkconfig` — the exact resolved config used, with two fixes applied on top of Freenove's plain defaults:
  - `CONFIG_SR_WN_WN9S_HIESP=y` (wake word — plain default is the Mandarin `NIHAOXIAOZHI` model, not "Hi, ESP"; the "S" variant is a newer/smaller model that tested faster than the plain `WN9_HIESP`)
  - `CONFIG_LANGUAGE_EN_US=y` (system UI language — plain default is `ZH_CN`, causes Chinese notification/status text)
- `custom-source/` — Grey's own code changes on top of Freenove's fork (not vendored upstream source). Currently just the push-OTA server — see its own README for what it does and how to reapply it to a fresh checkout.

## Updating the firmware over the network (no BOOT button needed)

As of the push-OTA server (see `custom-source/`), updates after the initial flash don't need physical access to the board:

```bash
curl -X POST -H "X-OTA-Token: <your-token-from-ota_push_server.cc>" --data-binary @firmware/xiaozhi.bin http://<device-ip>:8080/update
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
