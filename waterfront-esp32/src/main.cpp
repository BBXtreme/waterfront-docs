/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Initializes LittleFS, loads config, sets up tasks for MQTT, OTA, factory reset, and power management.
 *       Main loop handles MQTT, OTA, LTE power management, and periodic power checks for deep sleep.
 *       Edge cases: config load failure, task creation failure, power conditions.
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
#include <ArduinoOTA.h>
#include <Preferences.h>

// Include other headers as needed

// Firmware version define
const char* FW_VERSION = "0.9.2-beta";

// Factory reset task: Monitors GPIO 0 for long press to trigger factory reset.
// Resets config and NVS, publishes event, and restarts ESP32.
// Edge cases: button debounce, MQTT not connected, NVS erase failure.
void factory_reset_task(void *pvParameters) {
    const int RESET_BUTTON_PIN = 0;  // GPIO 0 (boot button)
    const unsigned long RESET_HOLD_TIME_MS = 5000;  // 5 seconds hold time
    pinMode(RESET_BUTTON_PIN, INPUT_PULLUP);  // Pull-up for active low
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    unsigned long pressStartTime = 0;
    bool buttonPressed = false;

    while (1) {
        esp_task_wdt_reset();  // Reset watchdog to prevent timeout
        int buttonState = digitalRead(RESET_BUTTON_PIN);
        if (buttonState == LOW) {  // Button pressed (active low)
            if (!buttonPressed) {
                pressStartTime = millis();
                buttonPressed = true;
                ESP_LOGI("MAIN", "Reset button pressed, hold for 5 seconds to trigger factory reset");
            } else if (millis() - pressStartTime > RESET_HOLD_TIME_MS) {
                // Factory reset triggered after 5s hold
                ESP_LOGW("MAIN", "Factory reset triggered by long-press on GPIO 0");
                // Publish event to MQTT for remote monitoring
                if (mqttClient.connected()) {
                    DynamicJsonDocument doc(256);
                    doc["event"] = "factory_reset";
                    doc["timestamp"] = millis();
                    String payload;
                    serializeJson(doc, payload);
                    char topic[96];
                    int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
                    if (len < sizeof(topic)) {
                        mqttClient.publish(topic, payload.c_str(), false);
                        ESP_LOGI("MAIN", "Published factory reset event");
                    } else {
                        ESP_LOGE("MAIN", "Topic too long, skipping publish");
                    }
                } else {
                    ESP_LOGW("MAIN", "MQTT not connected, skipping factory reset publish");
                }
                // Remove config file and erase NVS for clean reset
                if (!LittleFS.remove("/config.json")) {
                    ESP_LOGE("MAIN", "Failed to remove config file");
                }
                if (nvs_flash_erase() != ESP_OK) {
                    ESP_LOGE("MAIN", "Failed to erase NVS");
                }
                ESP_LOGI("MAIN", "Config removed and NVS erased, restarting...");
                delay(1000);  // Brief delay before restart
                esp_restart();
            }
        } else {
            buttonPressed = false;  // Reset state on release
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms for responsiveness
    }
}

// Overdue check task: Periodically checks for overdue rentals and triggers auto-lock.
// Edge cases: no active timers, MQTT publish failure.
void overdue_check_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        checkOverdue();  // Check and handle overdue rentals
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

// Debug task: Publishes health telemetry every 60s if debug mode is enabled.
// Includes uptime, heap, firmware version, battery, tasks, and reconnects.
// Edge cases: debug mode disabled, MQTT not connected.
void debug_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        if (g_config.debugMode) {
            // Collect telemetry data
            DynamicJsonDocument doc(512);
            doc["uptime"] = millis() / 1000;  // Uptime in seconds
            doc["heapFree"] = ESP.getFreeHeap();  // Free heap memory
            doc["fwVersion"] = FW_VERSION;  // Firmware version
            doc["batteryPercent"] = readBatteryLevel();  // Battery percentage
            doc["tasks"] = uxTaskGetNumberOfTasks();  // Number of active tasks
            doc["reconnects"] = getMqttReconnectCount();  // MQTT reconnect count
            String payload;
            serializeJson(doc, payload);
            // Publish to debug topic
            if (mqttClient.connected()) {
                char topic[96];
                int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
                if (len < sizeof(topic)) {
                    mqttClient.publish(topic, payload.c_str(), false);  // Not retained
                    ESP_LOGI("DEBUG", "Published debug telemetry: %s", payload.c_str());
                } else {
                    ESP_LOGE("DEBUG", "Topic too long, skipping debug publish");
                }
            } else {
                ESP_LOGW("DEBUG", "MQTT not connected, skipping debug publish");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(60000));  // Every 60 seconds
    }
}

// Function to read battery level: Reads ADC and maps to percentage.
// Assumes voltage divider for battery > 3.3V.
// Edge cases: ADC read failure, invalid values.
int readBatteryLevel() {
    int adcValue = analogRead(34);  // ADC pin for battery
    if (adcValue < 0 || adcValue > 4095) {
        ESP_LOGE("MAIN", "Invalid ADC value for battery: %d", adcValue);
        return 0;  // Fallback
    }
    // Map ADC (0-4095) to percentage (0-100), calibrate as needed
    int batteryPercent = map(adcValue, 0, 4095, 0, 100);
    return batteryPercent;
}

// Function to read solar voltage: Reads ADC and calculates voltage.
// Assumes 10k/3.3k divider for solar panel.
// Edge cases: ADC read failure, invalid values.
float readSolarVoltage() {
    int adcValue = analogRead(34);  // ADC pin for solar (same as battery in example)
    if (adcValue < 0 || adcValue > 4095) {
        ESP_LOGE("MAIN", "Invalid ADC value for solar: %d", adcValue);
        return 0.0f;  // Fallback
    }
    // Calculate voltage: ADC * (3.3V / 4095) * divider ratio
    float voltage = (adcValue / 4095.0) * 3.3 * (10.0 / 3.3);
    return voltage;
}

// Function to enter deep sleep: Configures wake sources and enters sleep.
// Wakes on timer (60s) or GPIO 0 low.
// Edge cases: already in sleep, wake source conflicts.
void enterDeepSleep() {
    ESP_LOGI("MAIN", "Entering deep sleep due to low power conditions");
    // Enable timer wakeup for 60 seconds
    esp_sleep_enable_timer_wakeup(60 * 1000000);
    // Enable GPIO wakeup on pin 0 low
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();  // Enter deep sleep
}

// Setup function: Initializes hardware, loads config, sets up tasks and OTA.
// Edge cases: LittleFS mount failure, config load failure, task creation failure.
void setup() {
    Serial.begin(115200);
    ESP_LOGI("MAIN", "WATERFRONT starting...");

    // Initialize task watchdog timer for stability (12s timeout)
    esp_task_wdt_init(12, true);
    esp_task_wdt_add(NULL);  // Add main task to watchdog

    // Initialize LittleFS for config storage
    if (!LittleFS.begin()) {
        fatal_error("LittleFS mount failed");
    } else {
        ESP_LOGI("MAIN", "LittleFS mounted");
    }

    // Load configuration from JSON, fallback to defaults
    if (!loadConfig()) {
        ESP_LOGW("MAIN", "Using defaults");
    }

    // Initialize compartment manager with loaded config
    compartment_init();

    // Initialize MQTT handler
    if (mqtt_init() != ESP_OK) {
        ESP_LOGE("MAIN", "MQTT init failed, but continuing");
    }

    // Initialize deposit logic
    deposit_init();

    // Initialize OTA with callbacks for remote monitoring
    ArduinoOTA.setHostname((g_config.mqtt.clientIdPrefix + "-ota").c_str());
    ArduinoOTA.setPassword("admin");  // Password for security
    ArduinoOTA.onStart([]() {
        String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
        ESP_LOGI("OTA", "Start updating %s", type.c_str());
        // Save current FW_VERSION to NVS before OTA
        Preferences preferences;
        preferences.begin("ota", false);
        preferences.putString("prev_version", FW_VERSION);
        preferences.end();
        ESP_LOGI("OTA", "Saved previous FW version to NVS: %s", FW_VERSION);
        // Publish OTA start event
        if (mqttClient.connected()) {
            DynamicJsonDocument doc(256);
            doc["event"] = "ota_start";
            doc["type"] = type;
            doc["timestamp"] = millis();
            String payload;
            serializeJson(doc, payload);
            char topic[96];
            int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
            if (len < sizeof(topic)) {
                mqttClient.publish(topic, payload.c_str(), false);
            }
        }
    });
    ArduinoOTA.onEnd([]() {
        ESP_LOGI("OTA", "End");
        // Clear saved version on successful OTA
        Preferences preferences;
        preferences.begin("ota", false);
        preferences.remove("prev_version");
        preferences.end();
        ESP_LOGI("OTA", "Cleared previous FW version from NVS after successful update");
        // Publish OTA end event
        if (mqttClient.connected()) {
            DynamicJsonDocument doc(256);
            doc["event"] = "ota_end";
            doc["timestamp"] = millis();
            String payload;
            serializeJson(doc, payload);
            char topic[96];
            int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
            if (len < sizeof(topic)) {
                mqttClient.publish(topic, payload.c_str(), false);
            }
        }
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        ESP_LOGI("OTA", "Progress: %u%%", (progress / (total / 100)));
        // Publish progress every 10% to avoid spam
        static unsigned int lastProgress = 0;
        unsigned int percent = progress / (total / 100);
        if (percent >= lastProgress + 10) {
            lastProgress = percent;
            if (mqttClient.connected()) {
                DynamicJsonDocument doc(256);
                doc["event"] = "ota_progress";
                doc["progress"] = percent;
                doc["timestamp"] = millis();
                String payload;
                serializeJson(doc, payload);
                char topic[96];
                int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
                if (len < sizeof(topic)) {
                    mqttClient.publish(topic, payload.c_str(), false);
                }
            }
        }
    });
    ArduinoOTA.onError([](ota_error_t error) {
        ESP_LOGE("OTA", "Error[%u]: ", error);
        // Log specific errors
        if (error == OTA_AUTH_ERROR) ESP_LOGE("OTA", "Auth Failed");
        else if (error == OTA_BEGIN_ERROR) ESP_LOGE("OTA", "Begin Failed");
        else if (error == OTA_CONNECT_ERROR) ESP_LOGE("OTA", "Connect Failed");
        else if (error == OTA_RECEIVE_ERROR) ESP_LOGE("OTA", "Receive Failed");
        else if (error == OTA_END_ERROR) ESP_LOGE("OTA", "End Failed");
        // Publish error event
        if (mqttClient.connected()) {
            DynamicJsonDocument doc(256);
            doc["event"] = "ota_error";
            doc["error_code"] = error;
            doc["timestamp"] = millis();
            String payload;
            serializeJson(doc, payload);
            char topic[96];
            int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
            if (len < sizeof(topic)) {
                mqttClient.publish(topic, payload.c_str(), false);
            }
        }
    });
    ArduinoOTA.begin();
    ESP_LOGI("OTA", "OTA initialized");

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

    // Create debug task if debug mode enabled
    if (g_config.debugMode) {
        task_ret = xTaskCreate(debug_task, "debug", 4096, NULL, 1, NULL);  // Low priority
        if (task_ret != pdPASS) {
            ESP_LOGE("MAIN", "Failed to create debug task");
        } else {
            ESP_LOGI("MAIN", "Debug task started");
        }
    }

    // Other initializations (WiFi, sensors, etc.) can be added here
}

// Main loop: Handles MQTT, OTA, LTE power, and power checks.
// Edge cases: MQTT loop failure, OTA handle failure, power check failures.
void loop() {
    // Process MQTT messages
    mqtt_loop();

    // Handle OTA updates
    ArduinoOTA.handle();

    // Manage LTE power based on conditions
    lte_power_management();

    // Other loop tasks can be added here

    // Periodic power check for deep sleep
    static unsigned long lastPowerCheck = 0;
    if (millis() - lastPowerCheck > 60000) {  // Every minute
        int batteryPercent = readBatteryLevel();
        float solarVoltage = readSolarVoltage();
        // Validate readings
        if (batteryPercent < 0 || batteryPercent > 100) {
            ESP_LOGE("MAIN", "Invalid battery percent: %d", batteryPercent);
            batteryPercent = 50;  // Fallback
        }
        if (solarVoltage < 0.0f || solarVoltage > 10.0f) {
            ESP_LOGE("MAIN", "Invalid solar voltage: %f", solarVoltage);
            solarVoltage = 5.0f;  // Fallback
        }
        // Enter deep sleep if battery low or solar low
        if (batteryPercent < g_config.system.batteryLowThresholdPercent || solarVoltage < g_config.system.solarVoltageMin) {
            enterDeepSleep();
        }
        lastPowerCheck = millis();
    }

    // Reset watchdog every loop iteration
    esp_task_wdt_reset();
}
