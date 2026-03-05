#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mqtt_client.h>

extern esp_mqtt_client_handle_t mqttClient;
extern bool mqttConnected;

/**
 * @brief Initializes the MQTT client with server and callback.
 */
esp_err_t mqtt_init();

/**
 * @brief MQTT event handler for handling incoming messages and connection events.
 * @param event The MQTT event structure.
 */
void event_handler(void* args, esp_event_base_t base, int32_t event_id, void* data);

/**
 * @brief Publishes status update to MQTT broker.
 */
void mqtt_publish_status();

/**
 * @brief Publishes slot-specific status.
 * @param slotId The slot ID.
 * @param jsonPayload The JSON payload string.
 */
void mqtt_publish_slot_status(int slotId, const char* jsonPayload);

/**
 * @brief Publishes retained status for a compartment.
 * @param compartmentId The compartment ID.
 * @param jsonPayload The JSON payload.
 */
void mqtt_publish_retained_status(int compartmentId, const char* jsonPayload);

/**
 * @brief Publishes acknowledgment for a compartment action.
 * @param compartmentId The compartment ID.
 * @param action The action performed.
 */
void mqtt_publish_ack(int compartmentId, const char* action);

/**
 * @brief Attempts to connect to MQTT broker.
 * @return True if connected, false otherwise.
 */
bool mqtt_connect();

/**
 * @brief Subscribes to MQTT topics.
 */
void mqtt_subscribe();

/**
 * @brief Maintains MQTT connection and processes messages.
 */
void mqtt_loop();

/**
 * @brief MQTT loop task.
 * @param pvParameters Task parameters.
 */
void mqtt_loop_task(void *pvParameters);

/**
 * @brief Returns the MQTT reconnect count for telemetry.
 */
int getMqttReconnectCount();

#endif // MQTT_CLIENT_H
