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
#include <esp_ota_ops.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <nvs_flash.h>

// Include other headers as needed

// Firmware version define
const char* FW_VERSION = "0.9.2-beta";

// Factory reset task
void factory_reset_task(void *pvParameters) {
    const int RESET_BUTTON_PIN = 0;  // GPIO 0 (boot button)
    const unsigned long RESET_HOLD_TIME_MS = 5000;  // 5 seconds
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);
    unsigned long pressStartTime = 0;
    bool buttonPressed = false;

    while (1) {
        int buttonState = digitalRead(RESET_BUTTON_PIN);
        if (buttonState == LOW) {  // Button pressed (active low)
            if (!buttonPressed) {
                pressStartTime = millis();
                buttonPressed = true;
            } else if (millis() - pressStartTime > RESET_HOLD_TIME_MS) {
                // Factory reset triggered
                ESP_LOGW("MAIN", "Factory reset triggered by long-press on GPIO 0");
                // Publish to MQTT debug topic
                DynamicJsonDocument doc(256);
                doc["event"] = "factory_reset";
                doc["timestamp"] = millis();
                String payload;
                serializeJson(doc, payload);
                char topic[96];
                snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
                mqttClient.publish(topic, payload.c_str(), false);
                // Remove config and erase NVS
                LittleFS.remove("/config.json");
                nvs_flash_erase();
                ESP_LOGI("MAIN", "Config removed and NVS erased, restarting...");
                delay(1000);
                esp_restart();
            }
        } else {
            buttonPressed = false;
        }
        esp_task_wdt_reset();  // Reset watchdog
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}

// Overdue check task
void overdue_check_task(void *pvParameters) {
    while (1) {
        checkOverdue();
        esp_task_wdt_reset();  // Reset watchdog
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

// Debug task for OTA-friendly logging
void debug_task(void *pvParameters) {
    while (1) {
        if (g_config.debugMode) {
            // Publish debug telemetry
            DynamicJsonDocument doc(512);
            doc["uptime"] = millis() / 1000;  // Uptime in seconds
            doc["freeHeap"] = ESP.getFreeHeap();
            doc["minFreeHeap"] = ESP.getMinFreeHeap();
            doc["wifiRSSI"] = WiFi.RSSI();
            doc["mqttConnected"] = mqttClient.connected();
            doc["otaPartition"] = esp_ota_get_running_partition()->label;
            doc["firmwareVersion"] = FW_VERSION;  // Firmware version
            String payload;
            serializeJson(doc, payload);
            char topic[96];
            snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
            mqttClient.publish(topic, payload.c_str(), false);  // Not retained
            ESP_LOGI("DEBUG", "Published debug telemetry: %s", payload.c_str());
        }
        esp_task_wdt_reset();  // Reset watchdog
        vTaskDelay(pdMS_TO_TICKS(1800000));  // Every 30 minutes (1800000 ms)
    }
}

void setup() {
    Serial.begin(115200);
    ESP_LOGI("MAIN", "WATERFRONT starting...");

    // Initialize task watchdog timer for stability (12s timeout)
    esp_task_wdt_init(12, true);  // 12s timeout, panic/restart on timeout
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

    // Create factory reset task
    BaseType_t task_ret = xTaskCreate(factory_reset_task, "factory_reset", 2048, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        fatal_error("Failed to create factory reset task");
    }

    // Create overdue check task
    task_ret = xTaskCreate(overdue_check_task, "overdue", 2048, NULL, 4, NULL);
    if (task_ret != pdPASS) {
        fatal_error("Failed to create overdue task");
    }

    // Create debug task if debugMode enabled
    if (g_config.debugMode) {
        task_ret = xTaskCreate(debug_task, "debug", 4096, NULL, 1, NULL);  // Low priority
        if (task_ret != pdPASS) {
            ESP_LOGE("MAIN", "Failed to create debug task");
        } else {
            ESP_LOGI("MAIN", "Debug task started");
        }
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
