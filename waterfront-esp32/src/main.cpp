/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Initializes LittleFS, loads config, starts tasks.
 */

#include <Arduino.h>
#include <LittleFS.h>
#include "config_loader.h"
#include "compartment_manager.h"
#include "mqtt_handler.h"
#include <esp_task_wdt.h>
#include "error_handler.h"
#include "deposit_logic.h"
// Include other headers as needed

// Overdue check task
void overdue_check_task(void *pvParameters) {
    while (1) {
        checkOverdue();
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

void setup() {
    Serial.begin(115200);
    ESP_LOGI("MAIN", "WATERFRONT starting...");

    // Initialize task watchdog timer for stability
    esp_task_wdt_init(15, true);  // 15s timeout, panic/restart on timeout
    esp_task_wdt_add(NULL);       // Subscribe main task

    // Initialize LittleFS early
    if (!LittleFS.begin()) {
        fatal_error("LittleFS mount failed");
    } else {
        ESP_LOGI("MAIN", "LittleFS mounted");
    }

    // Load runtime config
    if (!loadConfig()) {
        ESP_LOGW("MAIN", "Using defaults");
    }

    // Initialize compartment manager (now uses loaded config)
    compartment_init();

    // Initialize MQTT handler
    mqtt_init();

    // Initialize deposit logic
    deposit_init();

    // Create overdue check task
    BaseType_t task_ret = xTaskCreate(overdue_check_task, "overdue", 2048, NULL, 4, NULL);
    if (task_ret != pdPASS) {
        fatal_error("Failed to create overdue task");
    }

    // Other initializations (WiFi, sensors, etc.)
    // ...
}

void loop() {
    // MQTT loop
    mqtt_loop();

    // Other loop tasks
    // ...

    // Reset watchdog every loop iteration
    esp_task_wdt_reset();
}
