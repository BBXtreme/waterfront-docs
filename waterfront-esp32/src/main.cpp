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
 * @brief Global error handler for critical failures.
 * @param error_code The ESP error code.
 * @param message Descriptive message for the error.
 * @note Logs the error and restarts the ESP32.
 */
void global_error_handler(esp_err_t error_code, const char* message) {
    ESP_LOGE(TAG, "Critical error: %s (code: %d)", message, error_code);
    ESP_LOGI(TAG, "Restarting ESP32 in 5 seconds...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    esp_restart();
}

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
 * @brief FreeRTOS task for publishing debug traces if DEBUG_MODE is enabled.
 * @param pvParameters Unused parameter for FreeRTOS task.
 * @note Publishes debug information periodically to MQTT for monitoring.
 */
void debug_task(void *pvParameters) {
#if DEBUG_MODE
    while (1) {
        // Publish debug trace
        char topic[64];
        snprintf(topic, sizeof(topic), "waterfront/machine/%s/debug", MACHINE_ID);
        char payload[256];
        snprintf(payload, sizeof(payload), "{\"uptime\":%lu,\"freeHeap\":%d,\"wifiRSSI\":%d,\"mqttConnected\":%d}",
                 millis(), ESP.getFreeHeap(), WiFi.RSSI(), mqttClient.connected());
        mqttClient.publish(topic, payload, false);
        ESP_LOGI(TAG, "Published debug trace: %s", payload);
        vTaskDelay(pdMS_TO_TICKS(DEBUG_PUBLISH_INTERVAL_MS));
    }
#endif
}

/**
 * @brief Main application entry point.
 * @note Initializes NVS, event loop, components, and starts tasks. Runs main loop for periodic polling.
 */
void app_main() {
    ESP_LOGI(TAG, "WATERFRONT ESP32 starting...");

    // Initialize task watchdog timer for stability
    esp_task_wdt_init(10, true);  // 10-second timeout, panic on timeout

    // Initialize NVS with error handling
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        global_error_handler(ret, "Failed to initialize NVS");
    }

    // Initialize event loop with error handling
    ret = esp_event_loop_create_default();
    if (ret != ESP_OK) {
        global_error_handler(ret, "Failed to create event loop");
    }

    // Initialize components with error checks
    wifi_init();  // Assuming wifi_init returns esp_err_t or handle internally
    mqtt_init();
    relay_init();
    sensor_init();
    deposit_init();

    // Create sensor monitor task with error check
    BaseType_t task_ret = xTaskCreate(sensor_monitor_task, "sensor_monitor", 4096, NULL, 1, NULL);
    if (task_ret != pdPASS) {
        global_error_handler(ESP_FAIL, "Failed to create sensor monitor task");
    }

    // Create debug task if DEBUG_MODE is enabled
#if DEBUG_MODE
    task_ret = xTaskCreate(debug_task, "debug_task", 4096, NULL, 1, NULL);
    if (task_ret != pdPASS) {
        global_error_handler(ESP_FAIL, "Failed to create debug task");
    }
#endif

    ESP_LOGI(TAG, "All components initialized. Entering main loop.");

    // Main loop (simple polling for now; can be replaced with tasks)
    while (1) {
        // Check battery voltage
        float voltage = analogRead(BATTERY_ADC_PIN) * (3.3 / 4095.0) * 2;  // Assuming divider
        if (voltage < 3.3) {
            ESP_LOGW(TAG, "Low battery: %.2f V, skipping non-essential tasks", voltage);
            // Skip sensor polling, gate tasks, provisioning
            vTaskDelay(pdMS_TO_TICKS(SENSOR_POLL_INTERVAL_MS));
            continue;
        }

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
