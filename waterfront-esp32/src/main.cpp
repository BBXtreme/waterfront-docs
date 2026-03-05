/**
 * @file main.cpp
 * @brief Main entry point for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Initializes SPIFFS, loads config, sets up tasks for MQTT, OTA, factory reset, and power management.
 *       Main loop handles MQTT, OTA, LTE power management, and periodic power checks for deep sleep.
 *       Edge cases: config load failure, task creation failure, power conditions.
 */

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include <esp_spiffs.h>
#include <esp_https_ota.h>
#include <esp_http_client.h>
#include <cJSON.h>
#include <mqtt_client.h>
#include <esp_task_wdt.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_sleep.h>
#include "config_loader.h"
#include "compartment_manager.h"
#include "mqtt_handler.h"
#include <esp_timer.h>
#include "error_handler.h"
#include "deposit_logic.h"
#include "logger.h"

// Include other headers as needed

// Firmware version define
const char* FW_VERSION = "0.9.2-beta"; ///< Firmware version string

// Deep sleep profiling variables
static unsigned long awakeStartTime = 0;
static unsigned long totalAwakeTime = 0;
static unsigned int wakeUpCount = 0;
static esp_sleep_wakeup_cause_t lastWakeUpCause = ESP_SLEEP_WAKEUP_UNDEFINED;

// ADC configuration for power readings
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6  // GPIO 34
#define SOLAR_ADC_CHANNEL ADC_CHANNEL_7    // GPIO 35
#define ADC_ATTEN ADC_ATTEN_DB_11          // 0-3.9V range
#define ADC_WIDTH ADC_WIDTH_BIT_12         // 12-bit resolution
static esp_adc_cal_characteristics_t adc_chars;

// Initialize ADC for power readings
void adc_init() {
    adc1_config_width(ADC_WIDTH);
    adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN);
    adc1_config_channel_atten(SOLAR_ADC_CHANNEL, ADC_ATTEN);
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, &adc_chars);  // 1100mV Vref
    ESP_LOGI("ADC", "Initialized for battery and solar readings");
}

// Read battery level as percentage (0-100)
int readBatteryLevel() {
    int adc_raw = adc1_get_raw(BATTERY_ADC_CHANNEL);
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);
    // Assume battery voltage 3.0V (0%) to 4.2V (100%)
    float voltage_v = voltage_mv / 1000.0f;
    int percent = (int)((voltage_v - 3.0f) / (4.2f - 3.0f) * 100.0f);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

// Read solar voltage in volts
float readSolarVoltage() {
    int adc_raw = adc1_get_raw(SOLAR_ADC_CHANNEL);
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);
    return voltage_mv / 1000.0f;
}

// Enter deep sleep to conserve power
void enterDeepSleep() {
    ESP_LOGI("SLEEP", "Entering deep sleep for power conservation");
    // Configure wake-up sources
    esp_sleep_enable_timer_wakeup(3600000000ULL);  // Wake up every 1 hour (adjust as needed)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);   // Wake up on GPIO 0 low (reset button)
    // Disable WiFi and Bluetooth to save power
    esp_wifi_stop();
    esp_bt_controller_disable();
    // Enter deep sleep
    esp_deep_sleep_start();
}

/**
 * @brief Factory reset task: Monitors GPIO 0 for long press to trigger factory reset.
 *        Resets config and NVS, publishes event, and restarts ESP32.
 *        Edge cases: button debounce, MQTT not connected, NVS erase failure.
 * @param pvParameters Task parameters (unused).
 */
void factory_reset_task(void *pvParameters) {
    const int RESET_BUTTON_PIN = 0;  // GPIO 0 (boot button)
    const unsigned long RESET_HOLD_TIME_MS = 5000;  // 5 seconds hold time
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << RESET_BUTTON_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    unsigned long pressStartTime = 0;
    bool buttonPressed = false;

    while (1) {
        esp_task_wdt_reset();  // Reset watchdog to prevent timeout
        int buttonState = gpio_get_level((gpio_num_t)RESET_BUTTON_PIN);
        if (buttonState == 0) {  // Button pressed (active low)
            if (!buttonPressed) {
                pressStartTime = esp_timer_get_time() / 1000;
                buttonPressed = true;
                ESP_LOGI("MAIN", "Reset button pressed, hold for 5 seconds to trigger factory reset");
            } else if ((esp_timer_get_time() / 1000) - pressStartTime > RESET_HOLD_TIME_MS) {
                // Factory reset triggered after 5s hold
                ESP_LOGW("MAIN", "Factory reset triggered by long-press on GPIO 0");
                // Publish event to MQTT for remote monitoring
                if (mqttConnected) {
                    cJSON *doc = cJSON_CreateObject();
                    cJSON_AddStringToObject(doc, "event", "factory_reset");
                    cJSON_AddNumberToObject(doc, "timestamp", esp_timer_get_time() / 1000);
                    char *payload = cJSON_PrintUnformatted(doc);
                    char topic[96];
                    vPortEnterCritical(&g_configMutex);
                    int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug, g_config.location.code);
                    vPortExitCritical(&g_configMutex);
                    if (len < sizeof(topic)) {
                        esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);
                        ESP_LOGI("MAIN", "Published factory reset event");
                    } else {
                        ESP_LOGE("MAIN", "Topic too long, skipping publish");
                    }
                    cJSON_free(payload);
                    cJSON_Delete(doc);
                } else {
                    ESP_LOGW("MAIN", "MQTT not connected, skipping factory reset publish");
                }
                // Remove config file and erase NVS for clean reset
                if (esp_spiffs_remove("/spiffs/config.json") != ESP_OK) {
                    ESP_LOGE("MAIN", "Failed to remove config file");
                }
                if (nvs_flash_erase() != ESP_OK) {
                    ESP_LOGE("MAIN", "Failed to erase NVS");
                }
                ESP_LOGI("MAIN", "Config removed and NVS erased, restarting...");
                vTaskDelay(pdMS_TO_TICKS(1000));  // Brief delay before restart
                esp_restart();
            }
        } else {
            buttonPressed = false;  // Reset state on release
        }
        vTaskDelay(pdMS_TO_TICKS(100));  // Check every 100ms for responsiveness
    }
}

/**
 * @brief Overdue check task: Periodically checks for overdue rentals and triggers auto-lock.
 *        Edge cases: no active timers, MQTT publish failure.
 * @param pvParameters Task parameters (unused).
 */
void overdue_check_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        checkOverdue();  // Check and handle overdue rentals
        vTaskDelay(pdMS_TO_TICKS(10000));  // Check every 10 seconds
    }
}

/**
 * @brief Debug task: Publishes health telemetry every 60s if debug mode is enabled.
 *        Includes uptime, heap, firmware version, battery, tasks, and reconnects.
 *        Edge cases: debug mode disabled, MQTT not connected.
 * @param pvParameters Task parameters (unused).
 */
void debug_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        vPortEnterCritical(&g_configMutex);
        bool debugEnabled = g_config.system.debugMode;
        char locationSlug[32];
        strcpy(locationSlug, g_config.location.slug);
        char locationCode[32];
        strcpy(locationCode, g_config.location.code);
        vPortExitCritical(&g_configMutex);
        if (debugEnabled) {
            // Collect telemetry data
            cJSON *doc = cJSON_CreateObject();
            cJSON_AddNumberToObject(doc, "uptime", esp_timer_get_time() / 1000000);  // Uptime in seconds
            cJSON_AddNumberToObject(doc, "heapFree", esp_get_free_heap_size());  // Free heap memory
            cJSON_AddStringToObject(doc, "fwVersion", FW_VERSION);  // Firmware version
            cJSON_AddNumberToObject(doc, "batteryPercent", readBatteryLevel());  // Battery percentage
            cJSON_AddNumberToObject(doc, "tasks", uxTaskGetNumberOfTasks());  // Number of active tasks
            cJSON_AddNumberToObject(doc, "reconnects", getMqttReconnectCount());  // MQTT reconnect count
            cJSON_AddNumberToObject(doc, "totalAwakeTime", totalAwakeTime);  // Total awake time in ms
            cJSON_AddNumberToObject(doc, "wakeUpCount", wakeUpCount);  // Number of wake-ups
            cJSON_AddNumberToObject(doc, "lastWakeUpCause", lastWakeUpCause);  // Last wake-up cause
            char *payload = cJSON_PrintUnformatted(doc);
            // Publish to debug topic
            if (mqttConnected) {
                char topic[96];
                int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", locationSlug, locationCode);
                if (len < sizeof(topic)) {
                    esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);  // Not retained
                    ESP_LOGI("DEBUG", "Published debug telemetry: %s", payload);
                } else {
                    ESP_LOGE("DEBUG", "Topic too long, skipping debug publish");
                }
            } else {
                ESP_LOGW("DEBUG", "MQTT not connected, skipping debug publish");
            }
            cJSON_free(payload);
            cJSON_Delete(doc);
        }
        vTaskDelay(pdMS_TO_TICKS(60000));  // Every 60 seconds
    }
}

/**
 * @brief Setup function: Initializes hardware, loads config, sets up tasks and OTA.
 *        Edge cases: SPIFFS mount failure, config load failure, task creation failure.
 */
void app_main() {
    ESP_LOGI("MAIN", "WATERFRONT starting...");

    // Profile wake-up cause
    lastWakeUpCause = esp_sleep_get_wakeup_cause();
    wakeUpCount++;
    ESP_LOGI("MAIN", "Wake-up cause: %d, total wake-ups: %u", lastWakeUpCause, wakeUpCount);

    // Start awake time profiling
    awakeStartTime = esp_timer_get_time() / 1000;

    // Initialize task watchdog timer for stability (12s timeout)
    esp_task_wdt_init(12, true);
    esp_task_wdt_add(NULL);  // Add main task to watchdog

    // Initialize ADC for power readings
    adc_init();

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize SPIFFS for config storage
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };
    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE("MAIN", "SPIFFS mount failed, attempting graceful degradation");
        fatal_error("SPIFFS mount failed");
    } else {
        ESP_LOGI("MAIN", "SPIFFS mounted");
    }

    // Load configuration from JSON, fallback to defaults
    if (!loadConfig()) {
        ESP_LOGW("MAIN", "Using defaults");
    }

    // Initialize logger
    logger_init();

    // Initialize compartment manager with loaded config
    compartment_init();

    // Initialize MQTT handler
    if (mqtt_init() != ESP_OK) {
        ESP_LOGE("MAIN", "MQTT init failed, continuing with degraded mode");
        // Graceful degradation: continue without MQTT, log critical events locally
    }

    // Initialize deposit logic
    deposit_init();

    // Initialize OTA with callbacks for remote monitoring
    esp_http_client_config_t config = {
        .url = "https://example.com/firmware.bin",  // Placeholder; updated via MQTT
        .cert_pem = NULL,
        .skip_cert_common_name_check = true,
    };
    // OTA handle will be set later via MQTT

    // Create factory reset task
    BaseType_t task_ret = xTaskCreate(factory_reset_task, "factory_reset", 2048, NULL, 5, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create factory reset task, continuing without");
        // Graceful degradation: continue without factory reset
    }

    // Create overdue check task
    task_ret = xTaskCreate(overdue_check_task, "overdue", 2048, NULL, 4, NULL);
    if (task_ret != pdPASS) {
        ESP_LOGE("MAIN", "Failed to create overdue task, continuing without");
        // Graceful degradation: continue without overdue checks
    }

    // Create debug task if debug mode enabled
    vPortEnterCritical(&g_configMutex);
    bool debugEnabled = g_config.system.debugMode;
    vPortExitCritical(&g_configMutex);
    if (debugEnabled) {
        task_ret = xTaskCreate(debug_task, "debug", 4096, NULL, 1, NULL);  // Low priority
        if (task_ret != pdPASS) {
            ESP_LOGE("MAIN", "Failed to create debug task, continuing without");
            // Graceful degradation: continue without debug telemetry
        } else {
            ESP_LOGI("MAIN", "Debug task started");
        }
    }

    // Other initializations (WiFi, sensors, etc.) can be added here

    // Main loop for continuous operation
    while (1) {
        main_loop();
        vTaskDelay(pdMS_TO_TICKS(10));  // Small delay to yield
    }
}

/**
 * @brief Main loop: Handles MQTT, OTA, LTE power, and power checks.
 *        Edge cases: MQTT loop failure, OTA handle failure, power check failures.
 */
void main_loop() {
    esp_task_wdt_reset();  // Reset watchdog at start of loop

    // Process MQTT messages
    mqtt_loop();

    esp_task_wdt_reset();  // Reset after MQTT

    // Handle OTA updates (placeholder; triggered via MQTT)
    // esp_https_ota(&ota_config);  // Called when URL received

    esp_task_wdt_reset();  // Reset after OTA

    // Manage LTE power based on conditions
    lte_power_management();

    esp_task_wdt_reset();  // Reset after LTE

    // Other loop tasks can be added here

    // Periodic power check for deep sleep
    static unsigned long lastPowerCheck = 0;
    if ((esp_timer_get_time() / 1000) - lastPowerCheck > 60000) {  // Every minute
        ESP_LOGI("MAIN", "Performing power check");
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
        ESP_LOGI("MAIN", "Power check: battery %d%%, solar %.2fV", batteryPercent, solarVoltage);
        // Publish alert if low power
        if (batteryPercent < g_config.system.batteryLowThresholdPercent || solarVoltage < g_config.system.solarVoltageMin) {
            ESP_LOGW("MAIN", "Low power detected, publishing alert");
            if (mqttConnected) {
                cJSON *doc = cJSON_CreateObject();
                cJSON_AddStringToObject(doc, "alert", "low_power");
                cJSON_AddNumberToObject(doc, "batteryPercent", batteryPercent);
                cJSON_AddNumberToObject(doc, "solarVoltage", solarVoltage);
                cJSON_AddNumberToObject(doc, "timestamp", esp_timer_get_time() / 1000);
                char *payload = cJSON_PrintUnformatted(doc);
                char topic[96];
                vPortEnterCritical(&g_configMutex);
                int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/alert", g_config.location.slug, g_config.location.code);
                vPortExitCritical(&g_configMutex);
                if (len < sizeof(topic)) {
                    esp_mqtt_client_publish(mqttClient, topic, payload, 0, 1, 0);
                    ESP_LOGI("MAIN", "Published low power alert");
                } else {
                    ESP_LOGE("MAIN", "Alert topic too long, skipping publish");
                }
                cJSON_free(payload);
                cJSON_Delete(doc);
            } else {
                ESP_LOGW("MAIN", "MQTT not connected, skipping low power alert");
            }
        }
        // Enter deep sleep if battery low or solar low
        if (batteryPercent < g_config.system.batteryLowThresholdPercent || solarVoltage < g_config.system.solarVoltageMin) {
            ESP_LOGW("MAIN", "Entering deep sleep due to low power");
            enterDeepSleep();
        }
        lastPowerCheck = esp_timer_get_time() / 1000;
    }

    esp_task_wdt_reset();  // Reset watchdog at end of loop
}
