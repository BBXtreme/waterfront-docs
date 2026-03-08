/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>
#include <esp_https_ota.h>
#include <esp_http_client.h>
#include <esp_timer.h>              // for esp_timer_get_time()
#include <esp_task_wdt.h>           // for esp_task_wdt_reset()
#include "cJSON.h"
#include "MQTT_Client.h"
#include "config_loader.h"
#include "lte_manager.h"
#include "power_manager.h"

#define TAG "MAIN"

void app_main(void) {
    ESP_LOGI(TAG, "WATERFRONT ESP32 Kayak Rental Controller starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Initialize SPIFFS (for config, logs, etc.)
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    esp_vfs_spiffs_register(&conf);

    // Load config
    if (!loadConfig()) {
        ESP_LOGE(TAG, "Failed to load config - using defaults");
    }

    // Initialize components
    logger_init();
    wifi_init();
    mqtt_init();
    ota_init();
    compartment_init();
    relay_init();
    sensor_init();
    power_manager_init();
    lte_init();

    // Start tasks
    xTaskCreate(mqtt_loop_task, "mqtt_task", 4096, NULL, 5, NULL);
    // xTaskCreate(gate_task, "gate_task", 2048, NULL, 4, NULL);  // uncomment when ready

    ESP_LOGI(TAG, "All tasks started");

    // Main loop (supervision + deep sleep check)
    while (1) {
        esp_task_wdt_reset();

        // Manage LTE power
        lte_power_management();

        // Periodic power check
        static uint64_t lastPowerCheck = 0;
        uint64_t now = esp_timer_get_time() / 1000;
        if (now - lastPowerCheck > 60000) {  // every minute
            ESP_LOGI(TAG, "Power check");
            if (!power_manager_check_conditions()) {
                ESP_LOGW(TAG, "Low power → deep sleep");
                power_manager_stop_awake_profiling();
                power_manager_enter_deep_sleep();
            }
            lastPowerCheck = now;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // 100 ms loop
    }
}