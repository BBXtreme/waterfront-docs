// nimble.cpp - BLE provisioning using native IDF wifi_provisioning
#include "esp_log.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"

static const char* TAG = "BLE";

void startBLEProvisioning() {
    ESP_LOGI(TAG, "Starting BLE provisioning (IDF native)");

    wifi_prov_mgr_config_t cfg = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_HANDLER_FREE_BTDM
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_init(cfg));
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, "WATERFRONT-PROV", NULL));
}

void stopBLEProvisioning() {
    ESP_LOGI(TAG, "Stopping BLE provisioning");
}