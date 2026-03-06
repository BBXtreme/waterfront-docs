/**
 * @file power_manager.cpp
 * @brief Power management for deep sleep and low-power operation.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Optimizes for deep-sleep current <50 µA by disabling peripherals, using RTC memory, and configuring wake-ups.
 *       Includes dynamic thresholds, enhanced logging, and professional error handling.
 */

#include "power_manager.h"
#include "config_loader.h"
#include <esp_sleep.h>
#include <esp_log.h>
#include <esp_system.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_wifi.h>
#include <esp_bt.h>
#include <driver/gpio.h>
#include <string.h>

// ADC configuration for power readings (same as main.cpp)
#define BATTERY_ADC_CHANNEL ADC_CHANNEL_6  // GPIO 34
#define SOLAR_ADC_CHANNEL ADC_CHANNEL_7    // GPIO 35
#define ADC_ATTEN ADC_ATTEN_DB_11          // 0-3.9V range
#define ADC_WIDTH ADC_WIDTH_BIT_12         // 12-bit resolution
static esp_adc_cal_characteristics_t adc_chars;

// RTC memory for wake-up profiling (persists across deep sleep)
RTC_DATA_ATTR unsigned long totalAwakeTime;
RTC_DATA_ATTR uint32_t wakeUpCount;
RTC_DATA_ATTR int32_t lastWakeUpCause;
RTC_DATA_ATTR unsigned long awakeStartTime;

// Dynamic thresholds (can be updated via config or MQTT)
static int dynamicBatteryThreshold = 20;  // Default from config
static float dynamicSolarThreshold = 3.0f;  // Default from config

// Status strings
static const char* STATUS_OK = "OK";
static const char* STATUS_LOW_BATTERY = "LOW_BATTERY";
static const char* STATUS_LOW_SOLAR = "LOW_SOLAR";
static const char* STATUS_LOW_BOTH = "LOW_BOTH";

// Mutex for thread-safe access to dynamic thresholds
portMUX_TYPE powerMutex = portMUX_INITIALIZER_UNLOCKED;

// Initialize ADC for power readings
esp_err_t power_manager_init() {
    ESP_LOGI("POWER", "Initializing power manager");
    if (adc1_config_width(ADC_WIDTH) != ESP_OK) {
        ESP_LOGE("POWER", "Failed to configure ADC width");
        return ESP_FAIL;
    }
    if (adc1_config_channel_atten(BATTERY_ADC_CHANNEL, ADC_ATTEN) != ESP_OK) {
        ESP_LOGE("POWER", "Failed to configure battery ADC channel");
        return ESP_FAIL;
    }
    if (adc1_config_channel_atten(SOLAR_ADC_CHANNEL, ADC_ATTEN) != ESP_OK) {
        ESP_LOGE("POWER", "Failed to configure solar ADC channel");
        return ESP_FAIL;
    }
    if (esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH, 1100, &adc_chars) != ESP_ADC_CAL_VAL_EFUSE_VREF) {
        ESP_LOGW("POWER", "ADC calibration not ideal, using defaults");
    }

    // Set wake-up cause and increment count
    lastWakeUpCause = (int32_t)esp_sleep_get_wakeup_cause();
    wakeUpCount++;

    // Load initial thresholds from config
    vPortEnterCritical(&g_configMutex);
    dynamicBatteryThreshold = g_config.system.batteryLowThresholdPercent;
    dynamicSolarThreshold = g_config.system.solarVoltageMin;
    vPortExitCritical(&g_configMutex);

    ESP_LOGI("POWER", "ADC initialized for power readings, wake cause: %d, count: %u", lastWakeUpCause, wakeUpCount);
    return ESP_OK;
}

// Enter deep sleep with optimizations for <50 µA
void power_manager_enter_deep_sleep() {
    ESP_LOGI("POWER", "Entering deep sleep for power conservation");

    // Disable all non-essential peripherals to minimize current
    esp_err_t ret;
    ret = esp_wifi_stop();
    if (ret != ESP_OK) {
        ESP_LOGW("POWER", "Failed to stop WiFi: %s", esp_err_to_name(ret));
    }
    ret = esp_bt_controller_disable();
    if (ret != ESP_OK) {
        ESP_LOGW("POWER", "Failed to disable BT: %s", esp_err_to_name(ret));
    }
    adc_power_off();  // ADC off

    // Configure GPIO to minimize leakage: set all unused pins to input with no pull
    for (int pin = 0; pin < GPIO_NUM_MAX; pin++) {
        if (pin != g_config.sleep.sensorWakeupPin && pin != g_config.sleep.mqttWakeupPin && pin != 0) {  // Exclude wake pins and boot pin
            gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)pin, GPIO_FLOATING);  // No pull-up/down to avoid current draw
        }
    }

    // Configure wake-up sources
    vPortEnterCritical(&g_configMutex);
    bool enableSleep = g_config.sleep.enableDeepSleep;
    int timerSec = g_config.sleep.timerWakeupSec;
    int sensorPin = g_config.sleep.sensorWakeupPin;
    int mqttPin = g_config.sleep.mqttWakeupPin;
    vPortExitCritical(&g_configMutex);

    if (!enableSleep) {
        ESP_LOGI("POWER", "Deep sleep disabled in config, skipping");
        return;
    }

    // Timer wakeup
    ret = esp_sleep_enable_timer_wakeup(timerSec * 1000000ULL);  // Convert to microseconds
    if (ret != ESP_OK) {
        ESP_LOGE("POWER", "Failed to enable timer wakeup: %s", esp_err_to_name(ret));
        return;
    }

    // GPIO wakeup for sensor (rising edge, assume active high)
    ret = esp_sleep_enable_ext0_wakeup((gpio_num_t)sensorPin, 1);
    if (ret != ESP_OK) {
        ESP_LOGE("POWER", "Failed to enable sensor wakeup: %s", esp_err_to_name(ret));
    }

    // GPIO wakeup for MQTT (rising edge, assume active high)
    ret = esp_sleep_enable_ext1_wakeup(1ULL << mqttPin, ESP_EXT1_WAKEUP_ANY_HIGH);
    if (ret != ESP_OK) {
        ESP_LOGE("POWER", "Failed to enable MQTT wakeup: %s", esp_err_to_name(ret));
    }

    // Disable radio to save power (already done above, but ensure)
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);  // Keep RTC memory
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // Log before sleep (current draw should be ~50 µA or less after this)
    ESP_LOGI("POWER", "Deep sleep configured, current draw target <50 µA");

    // Enter deep sleep
    esp_deep_sleep_start();
}

// Check power conditions
bool power_manager_check_conditions() {
    int batteryPercent = power_manager_get_battery_percent();
    float solarVoltage = power_manager_get_solar_voltage();

    // Validate readings
    if (batteryPercent < 0 || batteryPercent > 100) {
        ESP_LOGE("POWER", "Invalid battery percent: %d", batteryPercent);
        batteryPercent = 50;  // Fallback
    }
    if (solarVoltage < 0.0f || solarVoltage > 10.0f) {
        ESP_LOGE("POWER", "Invalid solar voltage: %f", solarVoltage);
        solarVoltage = 5.0f;  // Fallback
    }

    ESP_LOGI("POWER", "Power check: battery %d%%, solar %.2fV", batteryPercent, solarVoltage);

    // Check thresholds
    vPortEnterCritical(&powerMutex);
    int batteryThreshold = dynamicBatteryThreshold;
    float solarThreshold = dynamicSolarThreshold;
    vPortExitCritical(&powerMutex);

    ESP_LOGI("POWER", "Thresholds: battery %d%%, solar %.2fV", batteryThreshold, solarThreshold);

    if (batteryPercent < batteryThreshold || solarVoltage < solarThreshold) {
        ESP_LOGW("POWER", "Low power detected, should enter deep sleep");
        return false;  // Enter sleep
    }

    return true;  // OK to continue
}

// Get battery percentage
int power_manager_get_battery_percent() {
    int adc_raw = adc1_get_raw(BATTERY_ADC_CHANNEL);
    if (adc_raw < 0) {
        ESP_LOGE("POWER", "Failed to read battery ADC");
        return 0;
    }
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);
    // Assume battery voltage 3.0V (0%) to 4.2V (100%)
    float voltage_v = voltage_mv / 1000.0f;
    int percent = (int)((voltage_v - 3.0f) / (4.2f - 3.0f) * 100.0f);
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    return percent;
}

// Get solar voltage
float power_manager_get_solar_voltage() {
    int adc_raw = adc1_get_raw(SOLAR_ADC_CHANNEL);
    if (adc_raw < 0) {
        ESP_LOGE("POWER", "Failed to read solar ADC");
        return 0.0f;
    }
    uint32_t voltage_mv = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);
    return voltage_mv / 1000.0f;
}

// Get total awake time
unsigned long power_manager_get_total_awake_time() {
    return totalAwakeTime;
}

// Get wake-up count
uint32_t power_manager_get_wake_up_count() {
    return wakeUpCount;
}

// Get last wake-up cause
esp_sleep_wakeup_cause_t power_manager_get_last_wake_up_cause() {
    return (esp_sleep_wakeup_cause_t)lastWakeUpCause;
}

// Start awake profiling
void power_manager_start_awake_profiling() {
    awakeStartTime = esp_timer_get_time() / 1000;
    ESP_LOGI("POWER", "Started awake profiling at %lu", awakeStartTime);
}

// Stop awake profiling and update total
void power_manager_stop_awake_profiling() {
    unsigned long currentTime = esp_timer_get_time() / 1000;
    unsigned long awakeDuration = currentTime - awakeStartTime;
    totalAwakeTime += awakeDuration;
    ESP_LOGI("POWER", "Stopped awake profiling, duration %lu ms, total %lu ms", awakeDuration, totalAwakeTime);
}

// Set dynamic thresholds
void power_manager_set_dynamic_thresholds(int batteryThreshold, float solarThreshold) {
    if (batteryThreshold < 0 || batteryThreshold > 100) {
        ESP_LOGE("POWER", "Invalid battery threshold: %d", batteryThreshold);
        return;
    }
    if (solarThreshold < 0.0f || solarThreshold > 10.0f) {
        ESP_LOGE("POWER", "Invalid solar threshold: %f", solarThreshold);
        return;
    }
    vPortEnterCritical(&powerMutex);
    dynamicBatteryThreshold = batteryThreshold;
    dynamicSolarThreshold = solarThreshold;
    vPortExitCritical(&powerMutex);
    ESP_LOGI("POWER", "Updated dynamic thresholds: battery %d%%, solar %.2fV", batteryThreshold, solarThreshold);
}

// Get status string
const char* power_manager_get_status_string() {
    int batteryPercent = power_manager_get_battery_percent();
    float solarVoltage = power_manager_get_solar_voltage();

    vPortEnterCritical(&powerMutex);
    int batteryThreshold = dynamicBatteryThreshold;
    float solarThreshold = dynamicSolarThreshold;
    vPortExitCritical(&powerMutex);

    bool lowBattery = batteryPercent < batteryThreshold;
    bool lowSolar = solarVoltage < solarThreshold;

    if (lowBattery && lowSolar) {
        return STATUS_LOW_BOTH;
    } else if (lowBattery) {
        return STATUS_LOW_BATTERY;
    } else if (lowSolar) {
        return STATUS_LOW_SOLAR;
    } else {
        return STATUS_OK;
    }
}
