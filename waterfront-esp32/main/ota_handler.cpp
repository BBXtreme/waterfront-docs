#include "ota_handler.h"
#include "config_loader.h"
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_log.h>
#include <esp_system.h>

#define TAG "OTA"

static esp_https_ota_config_t ota_config = { .http_config = NULL };

esp_err_t ota_init(void) {
    ESP_LOGI(TAG, "OTA handler initialized");
    return ESP_OK;
}

esp_err_t ota_perform_update(void) {
    ESP_LOGI(TAG, "OTA stub - implement real URL from config");
    return ESP_OK;
}