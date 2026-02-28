#include <stdio.h>
#include "esp_log.h"

// Required FreeRTOS includes for vTaskDelay and portTICK_PERIOD_MS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "WATERFRONT-ESP32";

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Hello from Waterfront ESP32 Kayak Controller!");
    printf("Build successful - base ESP-IDF setup ready for MQTT integration.\n");

    // Simple idle loop to keep the task alive (replace later with real logic)
    while (1) {
        ESP_LOGI(TAG, "Still alive...");
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // ~5 seconds delay
    }
}