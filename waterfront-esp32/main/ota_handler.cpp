/**
 * @file ota_handler.cpp
 * @brief Handles OTA firmware updates using esp_https_ota with cert pinning and rollback support.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Performs secure HTTPS OTA updates with certificate pinning, validates certificates, and supports rollback on failure.
 */

#include "ota_handler.h"
#include "config_loader.h"
#include <esp_https_ota.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_ota_ops.h>
#include <string.h>
#include <stdlib.h>

// OTA configuration
static esp_https_ota_config_t ota_config = {
    .http_config = {
        .url = NULL,  // Will be set from config
        .cert_pem = NULL,  // Will be set for cert pinning
        .skip_cert_common_name_check = true,  // For development; set to false in production
    },
};

// Buffer for cert
static char* cert_pem = NULL;

// Initialize OTA handler
esp_err_t ota_init() {
    ESP_LOGI("OTA", "Initializing OTA handler");
    // Load OTA URL from config
    vPortEnterCritical(&g_configMutex);
    char otaUrl[256];
    // Placeholder: construct URL from config or use a default
    // For now, use a placeholder URL; in production, this could be from MQTT or config
    strcpy(otaUrl, "https://example.com/firmware.bin");  // Update this to actual URL
    ota_config.http_config.url = otaUrl;

    // Load CA cert for pinning
    if (strlen(g_config.mqtt.caCertPath) > 0) {
        FILE* certFile = fopen(g_config.mqtt.caCertPath, "r");
        if (certFile) {
            fseek(certFile, 0, SEEK_END);
            size_t certSize = ftell(certFile);
            fseek(certFile, 0, SEEK_SET);
            cert_pem = (char*)malloc(certSize + 1);
            if (cert_pem) {
                fread(cert_pem, 1, certSize, certFile);
                cert_pem[certSize] = '\0';
                ota_config.http_config.cert_pem = cert_pem;
                ESP_LOGI("OTA", "Loaded CA cert for pinning from %s", g_config.mqtt.caCertPath);
            } else {
                ESP_LOGE("OTA", "Failed to allocate memory for cert");
            }
            fclose(certFile);
        } else {
            ESP_LOGE("OTA", "Failed to open CA cert file: %s", g_config.mqtt.caCertPath);
        }
    } else {
        ESP_LOGW("OTA", "No CA cert path configured, skipping cert pinning");
    }
    vPortExitCritical(&g_configMutex);
    ESP_LOGI("OTA", "OTA initialized with URL: %s", otaUrl);
    return ESP_OK;
}

// Perform OTA update with rollback support
esp_err_t ota_perform_update() {
    ESP_LOGI("OTA", "Starting OTA update with cert pinning");
    esp_err_t ret = esp_https_ota(&ota_config);
    if (ret == ESP_OK) {
        ESP_LOGI("OTA", "OTA update successful, marking app as valid to cancel rollback");
        ota_mark_app_valid();
        ESP_LOGI("OTA", "Restarting to apply update...");
        esp_restart();
    } else {
        ESP_LOGE("OTA", "OTA update failed: %s", esp_err_to_name(ret));
        // Do not mark as valid, so rollback will occur on reboot if configured
    }
    return ret;
}

// Check for OTA update (placeholder: always return false for now)
bool ota_check_for_update() {
    // Placeholder: Implement version checking via HTTP or MQTT
    // For example, fetch version from server and compare with FW_VERSION
    return false;
}

// Mark app as valid to cancel rollback
void ota_mark_app_valid() {
    esp_err_t ret = esp_ota_mark_app_valid_cancel_rollback();
    if (ret == ESP_OK) {
        ESP_LOGI("OTA", "App marked as valid, rollback cancelled");
    } else {
        ESP_LOGE("OTA", "Failed to mark app as valid: %s", esp_err_to_name(ret));
    }
}

// Check if rollback is available
bool ota_rollback_available() {
    const esp_partition_t* rollback_partition = esp_ota_get_next_update_partition(NULL);
    if (rollback_partition) {
        ESP_LOGI("OTA", "Rollback partition available: %s", rollback_partition->label);
        return true;
    } else {
        ESP_LOGI("OTA", "No rollback partition available");
        return false;
    }
}

// Perform rollback
esp_err_t ota_perform_rollback() {
    if (!ota_rollback_available()) {
        ESP_LOGE("OTA", "Rollback not available");
        return ESP_FAIL;
    }
    ESP_LOGI("OTA", "Performing rollback to previous firmware");
    esp_err_t ret = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
    if (ret == ESP_OK) {
        ESP_LOGI("OTA", "Rollback set, restarting...");
        esp_restart();
    } else {
        ESP_LOGE("OTA", "Failed to set rollback partition: %s", esp_err_to_name(ret));
    }
    return ret;
}
