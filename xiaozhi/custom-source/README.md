# Custom Source Changes

This is Grey's custom code on top of Freenove's `xiaozhi-esp32` fork — not vendored upstream source, just the delta. To apply to a fresh checkout of `C:\dev\xiaozhi-esp32-src` (or any clone of `github.com/Freenove/xiaozhi-esp32`):

## Push-style OTA server (`main/ota_push_server.h` / `.cc`)

Adds a local-network push-OTA endpoint, since the built-in `Ota` class (`main/ota.cc`) only ever *pulls* from a remote URL, and only checks once at boot. This lets you push a new build anytime the device is running and awake on WiFi, no BOOT button/USB required after the one-time flash that installs it.

**To apply:**
1. Copy `main/ota_push_server.h` and `main/ota_push_server.cc` (in this folder) into the target repo's `main/` directory.
2. In `main/CMakeLists.txt`, add `"ota_push_server.cc"` to the `SOURCES` list (next to `"ota.cc"`).
3. In `main/application.cc`:
   - Add `#include "ota_push_server.h"` near the top (next to the other includes)
   - After the line `CheckNewVersion(ota);` in `Application::Start()`, add:
     ```cpp
     OtaPushServer::GetInstance().Start();
     ```

**Usage** — device listens on port 8080. Push a new app binary (`xiaozhi.bin`, the plain app file — not `merged-binary.bin`) anytime it's on WiFi:

```bash
curl -X POST -H "X-OTA-Token: REDACTED-TOKEN" --data-binary @xiaozhi.bin http://<device-ip>:8080/update
```

Device writes it to the inactive OTA slot, verifies it, sets it as the boot partition, and reboots automatically — confirmed working end-to-end 2026-08-24.

**Limitations:**
- Only updates the app binary. The wake-word model (`srmodels.bin`) lives in a separate fixed partition and can't be updated this way — still needs a full manual flash.
- The `X-OTA-Token` header check is a basic guard against accidental/drive-by pushes on the LAN, not real security — it's a plaintext constant compiled into the binary (`OTA_PUSH_TOKEN` in `ota_push_server.cc`), extractable by anyone with the binary. Don't expose port 8080 outside your home network.
- No version check — unlike the pull-based `Ota` class, this always accepts and flashes whatever's pushed, since it's a deliberate manual action.

**Gotcha hit while adding this:** adding an explicit `PRIV_REQUIRES esp_http_server` to `main`'s `idf_component_register()` call in `CMakeLists.txt` broke an unrelated component (`opus.h: No such file or directory` from `78__esp-opus-encoder`) — `main` previously had *no* REQUIRES/PRIV_REQUIRES at all, which opts it into ESP-IDF's special "depend on everything implicitly" default. Adding any explicit REQUIRES list turns that off. Since `esp_http_server` is a standard built-in ESP-IDF component, it's already reachable under that same implicit default — don't add an explicit REQUIRES for it, the plain `idf_component_register()` call (unchanged) is enough.
