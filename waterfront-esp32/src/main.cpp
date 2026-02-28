/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Initializes all components, runs FreeRTOS tasks, and handles main loop for sensor monitoring and gate control.
 */

// main.cpp - Main entry point for WATERFRONT ESP32 Kayak Rental Controller
// This file initializes the ESP32, sets up WiFi, MQTT, and provisioning, and runs the main loop.
// It integrates all modules: WiFi/MQTT connectivity, provisioning, LTE fallback, and hardware control.
// The code uses FreeRTOS tasks for concurrent execution (e.g., MQTT handling in background).
// Framework: Arduino (for simplicity, but with ESP-IDF components via PlatformIO).

#include <esp_log.h>
#include <nvs_flash.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "config.h"
#include "wifi_manager.h"  // Assuming wifi_manager.h exists or will be added
#include "mqtt_client.h"
#include "relay_handler.h"
#include "return_sensor.h"
#include "deposit_logic.h"
#include "gate_control.h"
#include "nimble.h"  // For provisioning_task
#include "webui_server.h"  // For provisioning_task

static const char *TAG = "MAIN";

static char current_booking_id[32] = {0};

/**
 * @brief FreeRTOS task for monitoring kayak presence via ultrasonic sensor.
 * @param pvParameters Unused parameter for FreeRTOS task.
 * @note Polls sensor every 500ms, detects state changes, and publishes MQTT events.
 */
void sensor_monitor_task(void *pvParameters) {
    bool last_present = false;
    while (1) {
        float distance = sensor_get_distance();
        ESP_LOGI(TAG, "Sensor distance: %.2f cm", distance);
        bool present = sensor_is_kayak_present();
        if (present != last_present) {
            if (present) {
                // Kayak returned (absent → present)
                ESP_LOGI(TAG, "State change: absent → present (kayak returned)");
                deposit_on_return(&mqttClient);
                // Publish event
                char topic[64];
                snprintf(topic, sizeof(topic), "waterfront/slots/%s/event", SLOT_ID);
                char payload[128];
                snprintf(payload, sizeof(payload), "{\"event\":\"returned\",\"bookingId\":\"%s\"}", current_booking_id);
                mqttClient.publish(topic, payload);
            } else {
                // Kayak taken (present → absent)
                ESP_LOGI(TAG, "State change: present → absent (kayak taken)");
                deposit_on_take();
                // Publish event
                char topic[64];
                snprintf(topic, sizeof(topic), "waterfront/slots/%s/event", SLOT_ID);
                char payload[128];
                snprintf(payload, sizeof(payload), "{\"event\":\"taken\",\"bookingId\":\"%s\"}", current_booking_id);
                mqttClient.publish(topic, payload);
            }
            last_present = present;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief Main application entry point.
 * @note Initializes NVS, event loop, components, and starts tasks. Runs main loop for periodic polling.
 */
void app_main() {
    ESP_LOGI(TAG, "WATERFRONT ESP32 starting...");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize components
    wifi_init();  // From wifi_manager
    mqtt_init();
    relay_init();
    sensor_init();
    deposit_init();

    // Create sensor monitor task
    xTaskCreate(sensor_monitor_task, "sensor_monitor", 4096, NULL, 1, NULL);

    ESP_LOGI(TAG, "All components initialized. Entering main loop.");

    // Main loop (simple polling for now; can be replaced with tasks)
    while (1) {
        // Poll sensor periodically
        if (sensor_is_kayak_present()) {
            ESP_LOGI(TAG, "Kayak present");
            // Handle return logic here or in a task
        } else {
            ESP_LOGI(TAG, "Kayak not present");
        }

        // Handle gate tasks
        gate_task();

        // Handle provisioning tasks
        provisioning_task();

        vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_INTERVAL_MS));
    }
}
