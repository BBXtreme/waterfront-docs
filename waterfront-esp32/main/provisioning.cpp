/**
 * @file power_manager.cpp
 * @brief Power management for deep sleep and low-power operation.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Uses ESP-IDF ADC calibration API.
 *       Deep sleep current target <50 µA.
 */

#include "power_manager.h"
#include "config_loader.h"
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
/* Using ESP-IDF v4 ADC calibration API */
#include <esp_wifi.h>
#include <esp_bt.h>
#include <driver/gpio.h>
#include <string.h>

// ADC configuration
#define BATTERY_ADC_UNIT    ADC_UNIT_1
#define BATTERY_ADC_CH      ADC_CHANNEL_6   // GPIO34
#define SOLAR_ADC_UNIT      ADC_UNIT_1
#define SOLAR_ADC_CH        ADC_CHANNEL_7   // GPIO35
#define ADC_ATTEN           ADC_ATTEN_DB_11
#define ADC_BITWIDTH        ADC_BITWIDTH_12

// ADC calibration handle
static esp_adc_cal_characteristics_t *cali_handle = NULL;

// RTC memory (persists across deep sleep)
RTC_DATA_ATTR uint64_t totalAwakeTimeMs = 0;
RTC_DATA_ATTR uint32_t wakeUpCount = 0;
RTC_DATA_ATTR esp_sleep_wakeup_cause_t lastWakeUpCause = ESP_SLEEP_WAKEUP_UNDEFINED;
RTC_DATA_ATTR uint64_t awakeStartTimeUs = 0;

// Dynamic thresholds
static int dynamicBatteryThresholdPercent = 20;
static float dynamicSolarMinVoltage = 3.0f;
static portMUX_TYPE powerMutex = portMUX_INITIALIZER_UNLOCKED;

// Status strings
static const char* STATUS_OK = "OK";
static const char* STATUS_LOW_BATTERY = "LOW_BATTERY";
static const char* STATUS_LOW_SOLAR = "LOW_SOLAR";
static const char* STATUS_LOW_BOTH = "LOW_BOTH";

static const char* TAG = "POWER";

// Initialize ADC (oneshot + calibration)
esp_err_t power_manager_init(void) {
    ESP_LOGI(TAG, "Initializing power manager (ADC oneshot + calibration)");

    // Initialize oneshot ADC unit
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(BATTERY_ADC_CH, ADC_ATTEN);
    adc1_config_channel_atten(SOLAR_ADC_CH, ADC_ATTEN);

    cali_handle = (esp_adc_cal_characteristics_t *) calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, 1100, cali_handle);
    ESP_LOGI(TAG, "ADC initialized successfully");
    return ESP_OK;
}

// Read raw ADC value and convert to voltage (mV)
static esp_err_t read_adc_voltage(adc_unit_t unit, adc_channel_t ch, int* out_voltage_mv) {
    int raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, ch, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return ret;
    }

    if (cali_handle) {
        ret = adc_cali_raw_to_voltage(cali_handle, raw, out_voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Calibration conversion failed: %s", esp_err_to_name(ret));
            return ret;
        }
    } else {
        // Rough fallback (11 dB atten ≈ 0–3.9 V, 12-bit)
        *out_voltage_mv = (raw * 3900) / 4095;
    }

    return ESP_OK;
}

// Get battery percentage (simple linear approximation)
int power_manager_get_battery_percent(void) {
    int voltage_mv = 0;
    if (read_adc_voltage(BATTERY_ADC_UNIT, BATTERY_ADC_CH, &voltage_mv) != ESP_OK) {
        return -1;
    }

    // Example: 3.0V → 0%, 4.2V → 100% (LiPo typical)
    int percent = ((voltage_mv - 3000) * 100) / (4200 - 3000);
    percent = (percent < 0) ? 0 : (percent > 100 ? 100 : percent);

    ESP_LOGD(TAG, "Battery voltage: %dchannel_t ch, int* out_voltage_mv) {
    int raw = adc1_get_raw((adc1_channel_t)ch);

    if (cali_handle) {
        *out_voltage_mv = esp_adc_cal_raw_to_voltage(raw, cali_handle);n voltage;
}

// Enter deep sleep
void power_manager_enter_deep_sleep(uint64_t sleep_time_us) {
    power_manager_stop_awake_profiling();

    ESP_LOGI(TAG, "Entering deep sleep for %llu us", sleep_time_us);

    // Disable WiFi & BT
    esp_wifi_stop();
    esp_bt_controller_disable();

    // Configure wake-up sources (example: timer + GPIO)
    esp_sleep_enable_timer_wakeup(sleep_time_us);
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_XX, 1);  // add real GPIO wake if needed

    esp_deep_sleep_start();
}

// Start awake time measurement
void power_manager_start_awake_profil
    awakeStartTimeUs = esp_timer_get_time();
    ESP_LOGD(TAG, "Awake profiling started");
}

// Stop and accumulate awake time
void power_manager_stop_awake_profiling(void) {
    uint64_t now = esp_timer_get_time();
    uint64_t duration_us = now - awakeStartTimeUs;
    totalAwakeTimeMs += duration_us / 1000;

    ESP_LOGI(TAG, "Awake duration: %llu us, total awake: %llu ms", duration_us, totalAwakeTimeMs);
}

// Get last wake-up cause
esp_sleep_wakeup_cause_t power_manae_up_cause(void) {
    return lastWakeUpCause;
}

// Update thresholds from config or MQTT
void power_manager_set_dynamic_thresholds(int battery_percent, float solar_voltage) {
    portENTER_CRITICAL(&powerMutex);
    dynamicBatteryThresholdPercent = battery_percent;
    dynamicSolarMinVoltage = solar_voltage;
    portEXIT_CRITICAL(&powerMutex);

    ESP_LOGI(TAG, "Thresholds updated: battery %d%%, solar %.2fV",
             dynamicBatteryThresholdPercent, dynamicSolarMinVoltage);
}

// Get current status string
const char* power_manager_get_status_string(void) {
    int battery = power_manager_get_battery_percent();
    float solar = power_manager_get_solar_voltage();

    portENTER_CRITICAL(&powerMutex);
    int batt_thresh = dynamicBatteryThresholdPercent;
    float solar_thresh = dynamicSolarMinVoltage;
    portEXIT_CRITICAL(&powerMutex);

    if (battery < batt_thresh && solar < solar_thresh) return STATUS_LOW_BOTH;
    if (battery < batt_thresh) return STATUS_LOW_BATTERY;
    if (solar < solar_thresh) return STATUS_LOW_SOLAR;
    return STATUS_OK;
}