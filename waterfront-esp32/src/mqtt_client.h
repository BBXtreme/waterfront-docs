/**
 * @file mqtt_client.h
 * @brief Header for MQTT client management in WATERFRONT ESP32.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Provides extern declarations for MQTT client and basic functions.
 */

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <WiFiClient.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;

/**
 * @brief Initializes the MQTT client with server and callback.
 */
void mqtt_init();

/**
 * @brief MQTT callback function for handling incoming messages.
 * @param topic The MQTT topic of the received message.
 * @param payload The message payload as byte array.
 * @param length The length of the payload.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length);

/**
 * @brief Publishes status update to MQTT broker.
 */
void mqtt_publish_status();

#endif // MQTT_CLIENT_H
