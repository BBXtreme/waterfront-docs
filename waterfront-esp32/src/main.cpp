/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Initializes LittleFS, loads config.
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
        esp_task_wdt_reset();  // Reset watchdog at start of loop
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
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms
    }
}

// Overdue check task
void overdue_check_task(void *pvParameters) {
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog at start of loop
        checkOverdue();
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

// Debug task for remote health telemetry
void debug_task(void *pvParameters) {
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog at start of loop
        if (g_config.debugMode) {
            // Publish debug telemetry
            DynamicJsonDocument doc(512);
            doc["uptime"] = millis() / 1000;  // Uptime in seconds
            doc["heapFree"] = ESP.getFreeHeap();
            doc["fwVersion"] = FW_VERSION;
            doc["batteryPercent"] = readBatteryLevel();
            doc["tasks"] = uxTaskGetNumberOfTasks();
            doc["reconnects"] = getMqttReconnectCount();
            String payload;
            serializeJson(doc, payload);
            char topic[96];
            snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
            mqttClient.publish(topic, payload.c_str(), false);  // Not retained
            ESP_LOGI("DEBUG", "Published debug telemetry: %s", payload.c_str());
        }
        vTaskDelay(pdMS_TO_TICKS(60000));  // Every 60 seconds (60000 ms)
    }
}

// Function to read battery level (ADC)
int readBatteryLevel() {
    // Assume ADC pin from config (e.g., GPIO 34)
    // Add voltage divider for battery > 3.3V
    int adcValue = analogRead(34);  // Example pin
    // Convert to percentage (calibrate based on hardware)
    int batteryPercent = map(adcValue, 0, 4095, 0, 100);
    return batteryPercent;
}

// Function to read solar voltage
float readSolarVoltage() {
    // Assume ADC pin from config (e.g., GPIO 34)
    int adcValue = analogRead(34);  // Example pin
    // Convert to voltage (calibrate based on divider)
    float voltage = (adcValue / 4095.0) * 3.3 * (10.0 / 3.3);  // Example divider 10k/3.3k
    return voltage;
}

// Function to enter deep sleep
void enterDeepSleep() {
    ESP_LOGI("MAIN", "Entering deep sleep due to low solar voltage");
    // Configure wake sources: timer (60 s) or GPIO (button)
    esp_sleep_enable_timer_wakeup(60 * 1000000);  // 60 seconds
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // Wake on GPIO 0 low
    esp_deep_sleep_start();
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

    // LTE power management
    lte_power_management();

    // Other loop tasks
    // ...

    // Check solar voltage periodically
    static unsigned long lastSolarCheck = 0;
    if (millis() - lastSolarCheck > 60000) {  // Every minute
        float solarVoltage = readSolarVoltage();
        if (solarVoltage < g_config.system.solarVoltageMin) {
            enterDeepSleep();
        }
        lastSolarCheck = millis();
    }

    // Reset watchdog every loop iteration
    esp_task_wdt_reset();
}
