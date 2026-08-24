#ifndef _OTA_PUSH_SERVER_H
#define _OTA_PUSH_SERVER_H

#include <esp_http_server.h>

// Push-style OTA over the local network: POST a raw app .bin to /update
// and the device flashes it into the inactive OTA slot and reboots.
// Complements the existing pull-based Ota class (ota.h), which only
// checks for updates once at boot against a remote URL. This lets you
// push a build anytime the device is running, without touching BOOT/USB.
class OtaPushServer {
public:
    static OtaPushServer& GetInstance();
    void Start();

private:
    OtaPushServer() = default;
    ~OtaPushServer() = default;
    OtaPushServer(const OtaPushServer&) = delete;
    OtaPushServer& operator=(const OtaPushServer&) = delete;

    static esp_err_t HandleUpdate(httpd_req_t* req);

    httpd_handle_t server_ = nullptr;
};

#endif // _OTA_PUSH_SERVER_H
