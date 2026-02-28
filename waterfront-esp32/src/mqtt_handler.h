/**
 * @file mqtt_handler.h
 * @brief Header for advanced MQTT handling in WATERFRONT ESP32 slot system.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Declares functions for MQTT connections, subscriptions, and retained publishing.
 */

#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

/**
 * @brief Initializes MQTT handler and gate control.
 */
void mqtt_init();

/**
 * @brief Attempts to connect to MQTT broker with last-will.
 * @return True if connected, false otherwise.
 */
bool mqtt_connect();

/**
 * @brief Subscribes to slot-specific MQTT topics.
 */
void mqtt_subscribe();

/**
 * @brief Publishes retained status for a slot.
 * @param slotId The slot ID to publish for.
 * @param jsonPayload The JSON payload to publish.
 */
void mqtt_publish_retained_status(int slotId, const char* jsonPayload);

/**
 * @brief Publishes acknowledgment for a slot action.
 * @param slotId The slot ID.
 * @param action The action performed (e.g., "gate_opened").
 */
void mqtt_publish_ack(int slotId, const char* action);

/**
 * @brief MQTT callback for processing incoming messages.
 * @param topic The MQTT topic.
 * @param payload The message payload.
 * @param length The payload length.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length);

/**
 * @brief Maintains MQTT connection and processes messages.
 */
void mqtt_loop();

#endif // MQTT_HANDLER_H
