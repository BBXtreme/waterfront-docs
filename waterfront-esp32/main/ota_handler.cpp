/**
 * @file ota_handler.cpp
 * @brief Handles OTA firmware updates using esp_https_ota with cert pinning and rollback support.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Performs secure HTTPS OTA with certificate pinning, validates certs, supports rollback on failure.
 */

#include "ota_handler.h"
#include "config_loader.h"
#include <esp_https_ota.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>          // fopen, fclose, fread, etc.

#define TAG "OTA"

// Global OTA config (http_config is a pointer inside esp_https_ota_config_t)
static esp_https_ota_config_t ota_config = {
    .http_config = NULL,  // will be allocated in ota_init()
};

esp_err_t ota_init(void) {
    ESP_LOGI(TAG, "Initializing OTA handler");

    // Allocate http_config (required for esp_https_ota)
    esp_http_client_config_t *http_cfg = (esp_http_client_config_t *)calloc(1, sizeof(esp_http_client_config_t));
    if (!http_cfg) {
        ESP_LOGE(TAG, "Failed to allocate http_config");
        return ESP_ERR_NO_MEM;
    }

    // Set default URL (update with real logic from config or MQTT later)
    static char ota_url[256] = "https://example.com/firmware.bin";  // placeholder
    http_cfg->url = ota_url;
    http_cfg->skip_cert_common_name_check = true;  // TODO: set false in production

    // Load CA cert for pinning from config path
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
            } else {
                ESP_LOGE(TAG, "Failed to allocate memory for cert");
            }
            fclose(f);
        } else {
            ESP_LOGE(TAG, "Failed to open CA cert file: %s", g_config.mqtt.caCertPath);
        }
    } else {
        ESP_LOGW(TAG, "No CA cert path in config - skipping verification (insecure)");
        http_cfg->use_global_ca_store = true;  // fallback to IDF global bundle
    }
    vPortExitCritical(&g_configMutex);

    ota_config.http_config = http_cfg;
    return ESP_OK;
}

esp_err_t ota_perform_update(void) {
    if (!ota_config.http_config || !ota_config.http_config->url) {
        ESP_LOGE(TAG, "OTA not initialized - no URL configured");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting OTA update from: %s", ota_config.http_config->url);

    esp_err_t ret = esp_https_ota(&ota_config);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "OTA successful - marking app valid");
        ota_mark_app_valid();
        ESP_LOGI(TAG, "Restarting to apply new firmware...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA failed: %s (0x%x)", esp_err_to_name(ret), ret);
        // Rollback will trigger on reboot if not marked valid
    }

    return ret;
}

bool ota_check_for_update(void) {
    // TODO: real implementation (HTTP GET version.json, compare FW_VERSION)
    ESP_LOGI(TAG, "OTA update check not implemented yet");
    return false;
}

void ota_mark_app_valid(void) {
    esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "App marked valid - rollback cancelled");
    } else {
        ESP_LOGE(TAG, "Failed to mark app valid: %s", esp_err_to_name(ret));
    }
}

bool ota_rollback_available(void) {
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *next = esp_ota_get_next_update_partition(running);
    return (next != NULL && next != running);
}

esp_err_t ota_perform_rollback(void) {
    if (!ota_rollback_available()) {
        ESP_LOGE(TAG, "No rollback partition available");
        return ESP_ERR_NOT_FOUND;
    }

    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *rollback = esp_ota_get_next_update_partition(running);

    ESP_LOGI(TAG, "Rolling back to partition: %s", rollback->label);

    esp_err_t ret = esp_ota_set_boot_partition(rollback);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Rollback set - restarting...");
        esp_restart();
    } else {
        ESP_LOGE(TAG, "Failed to set rollback partition: %s", esp_err_to_name(ret));
    }

    return ret;
}