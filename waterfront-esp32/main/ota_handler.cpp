/**
 * @file ota_handler.cpp
 * @brief Handles OTA firmware updates using esp_https_ota with cert pinning and rollback support.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 */

#include "ota_handler.h"
#include "config_loader.h"
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define TAG "OTA"

// http_config is a pointer → we allocate it once
static esp_https_ota_config_t ota_config = {
    .http_config = NULL,
};

esp_err_t ota_init(void) {
    ESP_LOGI(TAG, "Initializing OTA handler");

    // Allocate http_config (required by esp_https_ota)
    esp_http_client_config_t *http_cfg = (esp_http_client_config_t *)calloc(1, sizeof(esp_http_client_config_t));
    if (!http_cfg) {
        ESP_LOGE(TAG, "Failed to allocate http_config");
        return ESP_ERR_NO_MEM;
    }

    // Default URL (update via config/MQTT later)
    static char ota_url[256] = "https://example.com/firmware.bin";
    http_cfg->url = ota_url;
    http_cfg->skip_cert_common_name_check = true;  // TODO: false in production

    vPortEnterCritical(&g_configMutex);
    if (strlen(g_config.mqtt.caCertPath) > 0) {
        FILE *f = fopen(g_config.mqtt.caCertPath, "r");
        if (f) {
            fseek(f, 0, SEEK_END);
            size_t size = ftell(f);
            fseek(f, 0, SEEK_SET);
            char *cert = (char *)malloc(size + 1);
            if (cert) {
                fread(cert, 1, size, f);
                cert[size] = '\0';
                http_cfg->cert_pem = cert;
                ESP_LOGI(TAG, "Loaded CA cert from %s (%zu bytes)", g_config.mqtt.caCertPath, size);
            }
            fclose(f);
        } else {
            ESP_LOGE(TAG, "Failed to open CA cert: %s", g_config.mqtt.caCertPath);
        }
    } else {
        ESP_LOGW(TAG, "No CA cert path - using global bundle");
        http_cfg->use_global_ca_store = true;
    }
    vPortExitCritical(&g_configMutex);

    ota_config.http_config = http_cfg;
    return ESP_OK;
}

esp_err_t ota_perform_update(void) {
    if (!ota_config.http_config || !ota_config.http_config->url) {
        ESP_LOGE(TAG, "OTA not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting OTA from: %s", ota_config.http_config->url);

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful - marking valid");
        ota_mark_app_valid();
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s (0x%x)", esp_err_to_name(ret), ret);
    }
    return ret;
}

// ... (rest of your functions: ota_check_for_update, ota_mark_app_valid, etc. remain unchanged)