#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

/**
 * @brief Initializes WiFi manager.
 */
void wifi_init();

/**
 * @brief WiFi event handler.
 * @param arg Event argument.
 * @param event_base Event base.
 * @param event_id Event ID.
 * @param event_data Event data.
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/**
 * @brief Quick MQTT check on wake-up for timer-based wake.
 */
void mqtt_quick_check();

#endif // WIFI_MANAGER_H
