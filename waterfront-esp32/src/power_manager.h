#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <esp_err.h>

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

#endif // POWER_MANAGER_H
