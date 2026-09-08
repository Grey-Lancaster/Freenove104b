#include "ota_push_server.h"

#include <esp_log.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <cstring>

#define TAG "OtaPushServer"
#define OTA_PUSH_SERVER_PORT 8080
#define OTA_RECV_BUF_SIZE 1024

// Basic shared-secret guard against accidental/drive-by pushes on the LAN.
// This is NOT real security (it ships in the compiled binary and can be
// extracted) -- change it to something private before relying on it, and
// don't expose this port outside your home network.
#define OTA_PUSH_TOKEN "544e9ffaa836ff3f3fa53e5f1e7557d2"

OtaPushServer& OtaPushServer::GetInstance() {
    static OtaPushServer instance;
    return instance;
}

void OtaPushServer::Start() {
    if (server_ != nullptr) {
        return;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = OTA_PUSH_SERVER_PORT;
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&server_, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OTA push server");
        server_ = nullptr;
        return;
    }

    httpd_uri_t update_uri = {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = HandleUpdate,
        .user_ctx = nullptr,
    };
    httpd_register_uri_handler(server_, &update_uri);

    ESP_LOGI(TAG, "OTA push server listening on port %d, POST a firmware .bin to /update", OTA_PUSH_SERVER_PORT);
}

esp_err_t OtaPushServer::HandleUpdate(httpd_req_t* req) {
    char token[64] = {0};
    if (httpd_req_get_hdr_value_str(req, "X-OTA-Token", token, sizeof(token)) != ESP_OK ||
        strcmp(token, OTA_PUSH_TOKEN) != 0) {
        ESP_LOGW(TAG, "OTA push rejected: missing/incorrect X-OTA-Token header");
        httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Missing or incorrect X-OTA-Token header");
        return ESP_FAIL;
    }

    if (req->content_len == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing body");
        return ESP_FAIL;
    }

    auto update_partition = esp_ota_get_next_update_partition(NULL);
    if (update_partition == nullptr) {
        ESP_LOGE(TAG, "Failed to get update partition");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No update partition");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Receiving OTA push, %u bytes, writing to partition %s at 0x%lx",
        (unsigned)req->content_len, update_partition->label, update_partition->address);

    esp_ota_handle_t update_handle = 0;
    if (esp_ota_begin(update_partition, OTA_WITH_SEQUENTIAL_WRITES, &update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "esp_ota_begin failed");
        return ESP_FAIL;
    }

    char buffer[OTA_RECV_BUF_SIZE];
    size_t remaining = req->content_len;
    size_t total_received = 0;
    auto last_log_time = esp_timer_get_time();

    while (remaining > 0) {
        size_t to_read = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        int received = httpd_req_recv(req, buffer, to_read);
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            ESP_LOGE(TAG, "Failed to receive OTA data (err %d)", received);
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }

        auto err = esp_ota_write(update_handle, buffer, received);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write failed: %s", esp_err_to_name(err));
            esp_ota_abort(update_handle);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
            return ESP_FAIL;
        }

        remaining -= received;
        total_received += received;

        if (esp_timer_get_time() - last_log_time >= 1000000) {
            ESP_LOGI(TAG, "OTA push progress: %u%% (%u/%u)",
                (unsigned)(total_received * 100 / req->content_len),
                (unsigned)total_received, (unsigned)req->content_len);
            last_log_time = esp_timer_get_time();
        }
    }

    esp_err_t err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
            err == ESP_ERR_OTA_VALIDATE_FAILED ? "Image validation failed" : "esp_ota_end failed");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to set boot partition");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "OK, rebooting...\n");

    ESP_LOGI(TAG, "OTA push update successful, rebooting in 2 seconds...");
    xTaskCreate([](void*) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    }, "ota_push_reboot", 2048, nullptr, 5, nullptr);

    return ESP_OK;
}
