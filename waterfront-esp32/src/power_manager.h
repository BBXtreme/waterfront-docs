#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <esp_err.h>
#include <esp_sleep.h>

/**
 * @brief Initializes power management.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t power_manager_init();

/**
 * @brief Enters deep sleep with configured wake-up sources.
 * Optimizes for <50 µA current draw.
 */
void power_manager_enter_deep_sleep();

/**
 * @brief Checks if power conditions allow continued operation.
 * @return true if OK, false if should sleep.
 */
bool power_manager_check_conditions();

/**
 * @brief Gets the current battery percentage.
 * @return Battery level in percent (0-100).
 */
int power_manager_get_battery_percent();

/**
 * @brief Gets the current solar voltage.
 * @return Solar voltage in volts.
 */
float power_manager_get_solar_voltage();

/**
 * @brief Gets the total awake time in milliseconds.
 * @return Total awake time.
 */
unsigned long power_manager_get_total_awake_time();

/**
 * @brief Gets the wake-up count.
 * @return Number of wake-ups.
 */
uint32_t power_manager_get_wake_up_count();

/**
 * @brief Gets the last wake-up cause.
 * @return Last wake-up cause.
 */
esp_sleep_wakeup_cause_t power_manager_get_last_wake_up_cause();

/**
 * @brief Starts awake time profiling.
 */
void power_manager_start_awake_profiling();

/**
 * @brief Stops awake time profiling and updates total.
 */
void power_manager_stop_awake_profiling();

/**
 * @brief Sets dynamic power thresholds based on external conditions.
 * @param batteryThreshold Battery low threshold in percent.
 * @param solarThreshold Solar voltage min in volts.
 */
void power_manager_set_dynamic_thresholds(int batteryThreshold, float solarThreshold);

/**
 * @brief Gets the current power status as a string.
 * @return Status string (e.g., "OK", "LOW_BATTERY", "LOW_SOLAR").
 */
const char* power_manager_get_status_string();

#endif // POWER_MANAGER_H
