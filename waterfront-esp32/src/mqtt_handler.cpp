/**
 * @file mqtt_handler.cpp
 * @brief Handles all MQTT subscriptions and callbacks for multi-compartment Waterfront booking system.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Uses retained topics for real-time compartment status sync with CRC32 checksums.
 */

// mqtt_handler.cpp - MQTT client handling for WATERFRONT ESP32
// This file manages MQTT connections, subscriptions, publishing, and callbacks.
// It integrates with the main loop for reliable IoT communication.
// Uses PubSubClient for Arduino compatibility.
// Updated for retained MQTT topics: subscribes to compartment status/command, publishes acks.
// Handles real-time compartment booking sync and gate control.
// Added CRC32 checksums for payload integrity.

#include "mqtt_handler.h"
#include "mqtt_topics.h"
#include "gate_control.h"
#include "config.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// CRC32 implementation (simple polynomial)
uint32_t computeCRC32(const char* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= (uint8_t)data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}

// External references
extern PubSubClient mqttClient;
extern WiFiClient wifiClient;

// Globals for location hierarchy
const char* LOCATION_TYPE = "locations";  // Fixed as "locations"
const char* LOCATION_CODE = "bremen-harbor-01";  // Load from config or NVS

// Compartment configuration (assume up to 10 compartments; adjust as needed)
#define MAX_COMPARTMENTS 10
int activeCompartments[MAX_COMPARTMENTS] = {1, 2, 3};  // Example: compartments 1,2,3 active; load from config

// Last-will topic and message for disconnect handling
#define MQTT_LAST_WILL_TOPIC MQTT_BASE_TOPIC "/esp32/disconnect"
#define MQTT_LAST_WILL_MESSAGE "{\"status\":\"disconnected\"}"

// Dual-SIM redundancy for LTE (if >1 location, switch SIMs on failure)
#define MAX_SIMS 2
int currentSim = 0;  // 0 or 1
// Note: Implement SIM switching logic in lte_manager.cpp if needed

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
 * @brief Subscribes to compartment-specific MQTT topics using wildcards.
 */
void mqtt_subscribe() {
    char topic[96];
    // Subscribe to status wildcard for this location
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_STATUS_WILDCARD, LOCATION_CODE);
    mqttClient.subscribe(topic, MQTT_QOS_STATUS);
    ESP_LOGI("MQTT", "Subscribed to %s", topic);
    // Subscribe to command wildcard for this location
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_COMMAND_WILDCARD, LOCATION_CODE);
    mqttClient.subscribe(topic, MQTT_QOS_COMMAND);
    ESP_LOGI("MQTT", "Subscribed to %s", topic);
}

/**
 * @brief Publishes retained status for a compartment with CRC32.
 * @param compartmentId The compartment ID to publish for.
 * @param jsonPayload The JSON payload to publish.
 */
void mqtt_publish_retained_status(int compartmentId, const char* jsonPayload) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, jsonPayload);  // Parse to add CRC
    uint32_t crc = computeCRC32(jsonPayload, strlen(jsonPayload));
    doc["crc"] = crc;
    String updatedPayload;
    serializeJson(doc, updatedPayload);
    char topic[96];
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_STATUS_FMT, LOCATION_CODE, compartmentId);
    mqttClient.publish(topic, updatedPayload.c_str(), MQTT_RETAIN_STATUS);
    ESP_LOGI("MQTT", "Published retained status to %s: %s", topic, updatedPayload.c_str());
}

/**
 * @brief Publishes acknowledgment for a compartment action with CRC32.
 * @param compartmentId The compartment ID.
 * @param action The action performed (e.g., "gate_opened").
 */
void mqtt_publish_ack(int compartmentId, const char* action) {
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"compartmentId\":%d,\"action\":\"%s\",\"timestamp\":%lu}", compartmentId, action, millis());
    uint32_t crc = computeCRC32(payload, strlen(payload));
    char fullPayload[256];
    snprintf(fullPayload, sizeof(fullPayload), "%s,\"crc\":%lu}", payload, crc);
    fullPayload[strlen(fullPayload) - 1] = '}';  // Fix JSON
    char topic[96];
    snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_ACK_FMT, LOCATION_CODE, compartmentId);
    mqttClient.publish(topic, fullPayload, MQTT_RETAIN_ACK);
    ESP_LOGI("MQTT", "Published ack to %s: %s", topic, fullPayload);
}

/**
 * @brief MQTT callback for processing incoming messages with CRC validation.
 * @param topic The MQTT topic.
 * @param payload The message payload.
 * @param length The payload length.
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    ESP_LOGI("MQTT", "Received on %s: %s at %lu", topic, msg.c_str(), millis());

    // Validate CRC
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        ESP_LOGE("MQTT", "JSON parse failed for status");
        return;
    }
    uint32_t receivedCrc = doc["crc"];
    doc.remove("crc");  // Remove for recompute
    String cleanMsg;
    serializeJson(doc, cleanMsg);
    uint32_t computedCrc = computeCRC32(cleanMsg.c_str(), cleanMsg.length());
    if (receivedCrc != computedCrc) {
        ESP_LOGE("MQTT", "CRC mismatch: received %lu, computed %lu", receivedCrc, computedCrc);
        return;
    }

    // Parse topic to extract compartmentId using sscanf for efficiency
    int compartmentId = -1;
    char locationType[16], locationCode[32];
    if (sscanf(topic, MQTT_BASE_TOPIC "/%15[^/]/%31[^/]/compartments/%d/status", locationType, locationCode, &compartmentId) == 3) {
        // Handle status message (retained sync)
        bool booked = doc["booked"];
        const char* gateState = doc["gateState"];
        ESP_LOGI("MQTT", "Compartment %d: booked=%d, gateState=%s", compartmentId, booked, gateState);
        if (booked && strcmp(gateState, "locked") == 0) {
            openCompartmentGate(compartmentId);
            mqtt_publish_ack(compartmentId, "gate_opened");
        }
    } else if (sscanf(topic, MQTT_BASE_TOPIC "/%15[^/]/%31[^/]/compartments/%d/command", locationType, locationCode, &compartmentId) == 3) {
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
            uint32_t crc = computeCRC32(payload, strlen(payload));
            char fullPayload[128];
            snprintf(fullPayload, sizeof(fullPayload), "%s,\"crc\":%lu}", payload, crc);
            fullPayload[strlen(fullPayload) - 1] = '}';  // Fix JSON
            char topic[96];
            snprintf(topic, sizeof(topic), MQTT_COMPARTMENT_ACK_FMT, LOCATION_CODE, compartmentId);
            mqttClient.publish(topic, fullPayload, MQTT_RETAIN_ACK);
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
