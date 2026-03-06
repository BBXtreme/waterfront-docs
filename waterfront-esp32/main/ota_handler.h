#ifndef OTA_HANDLER_H
#define OTA_HANDLER_H

#include <esp_err.h>

/**
 * @brief Initializes OTA handler.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t ota_init();

/**
 * @brief Performs OTA update from the configured URL with cert pinning and rollback support.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t ota_perform_update();

/**
 * @brief Checks if an OTA update is available (placeholder for future implementation).
 * @return true if update available, false otherwise.
 */
bool ota_check_for_update();

/**
 * @brief Marks the current app as valid to cancel rollback.
 */
void ota_mark_app_valid();

/**
 * @brief Checks if rollback is available.
 * @return true if rollback is possible, false otherwise.
 */
bool ota_rollback_available();

/**
 * @brief Performs rollback to previous firmware version.
 * @return ESP_OK on success, error code otherwise.
 */
esp_err_t ota_perform_rollback();

#endif // OTA_HANDLER_H
