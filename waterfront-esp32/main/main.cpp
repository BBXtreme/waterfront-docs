#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <esp_sleep.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>

#include "mqtt_client.h"
#include "config_loader.h"
#include "lte_manager.h"
#include "config.h"

#define TAG "MAIN"

void app_main(void) {
    ESP_LOGI(TAG, "WATERFRONT ESP32 starting...");

    nvs_flash_init();
    esp_vfs_spiffs_conf_t conf = { .base_path = "/spiffs", .max_files = 5, .format_if_mount_failed = true };
    esp_vfs_spiffs_register(&conf);

    if (!loadConfig()) ESP_LOGE(TAG, "Failed to load config");

    wifi_init();
    mqtt_init();
    ota_init();
    lte_init();

    xTaskCreate(mqtt_loop_task, "mqtt_task", 4096, NULL, 5, NULL);

    while (1) {
        esp_task_wdt_reset();
        lte_power_management();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}