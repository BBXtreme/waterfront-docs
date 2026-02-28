#include "wifi_manager.h"
#include "config_loader.h"
#include <ESPmDNS.h>
#include <WebServer.h>

// Adapted from original wifi_event_handler in mdb-slave-esp32s3.c
void wifi_event_handler(WiFiEvent_t event) {
    switch (event) {
        case WIFI_EVENT_STA_START:
            ESP_LOGI("WiFi", "STA started, connecting...");
            WiFi.begin(g_config.wifiProvisioning.fallbackSsid.c_str(), g_config.wifiProvisioning.fallbackPass.c_str());
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI("WiFi", "Connected to AP");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI("WiFi", "Disconnected, retrying...");
            WiFi.reconnect();
            break;
        default:
            break;
    }
}

void wifi_init() {
    WiFi.onEvent(wifi_event_handler);
    WiFi.begin(g_config.wifiProvisioning.fallbackSsid.c_str(), g_config.wifiProvisioning.fallbackPass.c_str());
}
