/**
 * @file mqtt_handler.cpp
 * @brief Handles all MQTT subscriptions and callbacks for multi-compartment Waterfront booking system.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Uses retained topics for real-time compartment status sync.
 */

// mqtt_handler.cpp - MQTT client handling for WATERFRONT ESP32
// This file manages MQTT connections, subscriptions, publishing, and callbacks.
// It integrates with the main loop for reliable IoT communication.
// Uses PubSubClient for Arduino compatibility.
// Updated for retained MQTT topics: subscribes to compartment status/command, publishes acks.
// Handles real-time compartment booking sync and gate control.

#include "mqtt_handler.h"
#include "mqtt_topics.h"
#include "gate_control.h"
#include "config.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// External references
extern PubSubClient mqttClient;
extern WiFiClient wifiClient;

// Compartment configuration (assume up to 10 compartments; adjust as needed)
#define MAX_COMPARTMENTS 10
int activeCompartments[MAX_COMPARTMENTS] = {1, 2, 3};  // Example: compartments 1,2,3 active; load from config

// Last-will topic and message for disconnect handling
#define MQTT_LAST_WILL_TOPIC MQTT_BASE_TOPIC "/esp32/disconnect"
#define MQTT_LAST_WILL_MESSAGE "{\"status\":\"disconnected\"}"

/**
 * @brief Initializes MQTT handler and gate control.
 */
void mqtt_init() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    gate_init();  // Initialize gate control
    ESP_LOGI("MQTT", "Initialized with broker %s:%d", MQTT_SERVER, MQTT_PORT);
}

/**
 * @brief Attempts to connect to MQTT broker with last-will.
 * @return True if connected, false otherwise.
 */
bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    ESP_LOGI("MQTT", "Connecting...");
    // Set last-will (PubSubClient syntax: connect(clientId, willTopic, willQoS, willRetain, willMessage))
    if (mqttClient.connect("KayakClient", MQTT_LAST_WILL_TOPIC, MQTT_QOS_STATUS, MQTT_RETAIN_STATUS, MQTT_LAST_WILL_MESSAGE)) {
        ESP_LOGI("MQTT", "Connected");
        mqtt_subscribe();
        return true;
    } else {
        ESP_LOGE("MQTT", "Failed, rc=%d", mqttClient.state());
        return false;
    }
}

/**
 * @brief Subscribes to compartment-specific MQTT topics.
 */
void mqtt_subscribe() {
    for (int i = 0; i < MAX_COMPARTMENTS; i++) {
        if (activeCompartments[i] > 0) {
            char topic[64];
            snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_STATUS_TOPIC, LOCATION_CODE, activeCompartments[i]);
            mqttClient.subscribe(topic, MQTT_QOS_STATUS);
            ESP_LOGI("MQTT", "Subscribed to %s", topic);
            snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_COMMAND_TOPIC, LOCATION_CODE, activeCompartments[i]);
            mqttClient.subscribe(topic, MQTT_QOS_COMMAND);
            ESP_LOGI("MQTT", "Subscribed to %s", topic);
        }
    }
}

/**
 * @brief Publishes retained status for a compartment.
 * @param compartmentId The compartment ID to publish for.
 * @param jsonPayload The JSON payload to publish.
 */
void mqtt_publish_retained_status(int compartmentId, const char* jsonPayload) {
    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_STATUS_TOPIC, LOCATION_CODE, compartmentId);
    mqttClient.publish(topic, jsonPayload, MQTT_RETAIN_STATUS);
    ESP_LOGI("MQTT", "Published retained status to %s: %s", topic, jsonPayload);
}

/**
 * @brief Publishes acknowledgment for a compartment action.
 * @param compartmentId The compartment ID.
 * @param action The action performed (e.g., "gate_opened").
 */
void mqtt_publish_ack(int compartmentId, const char* action) {
    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_ACK_TOPIC, LOCATION_CODE, compartmentId);
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"compartmentId\":%d,\"action\":\"%s\",\"timestamp\":%lu}", compartmentId, action, millis());
    mqttClient.publish(topic, payload, MQTT_RETAIN_ACK);
    ESP_LOGI("MQTT", "Published ack to %s: %s", topic, payload);
}

/**
 * @brief MQTT callback for processing incoming messages.
 * @param topic The MQTT topic.
 * @param payload The message payload.
 * @param length The payload length.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    ESP_LOGI("MQTT", "Received on %s: %s at %lu", topic, msg.c_str(), millis());

    // Parse topic to extract compartmentId using sscanf for efficiency
    int compartmentId = -1;
    char locationCode[32];
    if (sscanf(topic, MQTT_BASE_TOPIC "/locations/%31[^/]/compartments/%d/status", locationCode, &compartmentId) == 2) {
        // Handle status message (retained sync)
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, msg);
        if (error) {
            ESP_LOGE("MQTT", "JSON parse failed for status");
            return;
        }
        bool booked = doc["booked"];
        const char* gateState = doc["gateState"];
        ESP_LOGI("MQTT", "Compartment %d: booked=%d, gateState=%s", compartmentId, booked, gateState);
        if (booked && strcmp(gateState, "locked") == 0) {
            openCompartmentGate(compartmentId);
            mqtt_publish_ack(compartmentId, "gate_opened");
        }
    } else if (sscanf(topic, MQTT_BASE_TOPIC "/locations/%31[^/]/compartments/%d/command", locationCode, &compartmentId) == 2) {
        // Handle command message
        ESP_LOGI("MQTT", "Compartment %d command: %s", compartmentId, msg.c_str());
        if (msg == "open_gate") {
            openCompartmentGate(compartmentId);
            mqtt_publish_ack(compartmentId, "gate_opened");
        } else if (msg == "close_gate") {
            closeCompartmentGate(compartmentId);
            mqtt_publish_ack(compartmentId, "gate_closed");
        } else if (msg == "query_state") {
            const char* state = getCompartmentGateState(compartmentId);
            char payload[64];
            snprintf(payload, sizeof(payload), "{\"compartmentId\":%d,\"gateState\":\"%s\"}", compartmentId, state);
            char topic[64];
            snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_ACK_TOPIC, LOCATION_CODE, compartmentId);
            mqttClient.publish(topic, payload, MQTT_RETAIN_ACK);
        }
    }
}

/**
 * @brief Maintains MQTT connection and processes messages.
 */
void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqtt_connect();
    }
    mqttClient.loop();
}
