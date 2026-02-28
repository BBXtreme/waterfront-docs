// mqtt_handler.cpp - MQTT client handling for WATERFRONT ESP32
// This file manages MQTT connections, subscriptions, publishing, and callbacks.
// It integrates with the main loop for reliable IoT communication.
// Uses PubSubClient for Arduino compatibility.
// Updated for retained MQTT topics: subscribes to slot status/command, publishes acks.
// Handles real-time slot booking sync and gate control.

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

// Slot configuration (assume up to 10 slots; adjust as needed)
#define MAX_SLOTS 10
int activeSlots[MAX_SLOTS] = {1, 2, 3};  // Example: slots 1,2,3 active; load from config

// Last-will topic and message for disconnect handling
#define MQTT_LAST_WILL_TOPIC MQTT_BASE_TOPIC "/esp32/disconnect"
#define MQTT_LAST_WILL_MESSAGE "{\"status\":\"disconnected\"}"

// Initialize MQTT handler
void mqtt_init() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    ESP_LOGI("MQTT", "Initialized with broker %s:%d", MQTT_SERVER, MQTT_PORT);
}

// Connect to MQTT broker with last-will
bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    ESP_LOGI("MQTT", "Connecting...");
    // Set last-will
    if (mqttClient.connect("KayakClient", MQTT_LAST_WILL_TOPIC, MQTT_QOS_STATUS, MQTT_RETAIN_STATUS, MQTT_LAST_WILL_MESSAGE)) {
        ESP_LOGI("MQTT", "Connected");
        mqtt_subscribe();
        return true;
    } else {
        ESP_LOGE("MQTT", "Failed, rc=%d", mqttClient.state());
        return false;
    }
}

// Subscribe to slot topics
void mqtt_subscribe() {
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (activeSlots[i] > 0) {
            char topic[64];
            snprintf(topic, sizeof(topic), MQTT_SLOT_STATUS_TOPIC, activeSlots[i]);
            mqttClient.subscribe(topic, MQTT_QOS_STATUS);
            ESP_LOGI("MQTT", "Subscribed to %s", topic);
            snprintf(topic, sizeof(topic), MQTT_SLOT_COMMAND_TOPIC, activeSlots[i]);
            mqttClient.subscribe(topic, MQTT_QOS_COMMAND);
            ESP_LOGI("MQTT", "Subscribed to %s", topic);
        }
    }
}

// Publish retained status (for backend to sync)
void mqtt_publish_retained_status(int slotId, const char* jsonPayload) {
    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_SLOT_STATUS_TOPIC, slotId);
    mqttClient.publish(topic, jsonPayload, MQTT_RETAIN_STATUS);
    ESP_LOGI("MQTT", "Published retained status to %s: %s", topic, jsonPayload);
}

// Publish ack (not retained)
void mqtt_publish_ack(int slotId, const char* action) {
    char topic[64];
    snprintf(topic, sizeof(topic), MQTT_SLOT_ACK_TOPIC, slotId);
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"slotId\":%d,\"action\":\"%s\",\"timestamp\":%lu}", slotId, action, millis());
    mqttClient.publish(topic, payload, MQTT_RETAIN_ACK);
    ESP_LOGI("MQTT", "Published ack to %s: %s", topic, payload);
}

// MQTT callback for incoming messages
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    ESP_LOGI("MQTT", "Received on %s: %s at %lu", topic, msg.c_str(), millis());

    // Parse topic to extract slotId
    int slotId = -1;
    if (sscanf(topic, MQTT_BASE_TOPIC "/slots/%d/status", &slotId) == 1) {
        // Handle status message (retained sync)
        DynamicJsonDocument doc(512);
        deserializeJson(doc, msg);
        bool booked = doc["booked"];
        const char* gateState = doc["gateState"];
        if (booked && strcmp(gateState, "locked") == 0) {
            openGateServo(slotId);
            mqtt_publish_ack(slotId, "gate_opened");
        }
        // TODO: Update 7-segment/LED matrix with booking info
    } else if (sscanf(topic, MQTT_BASE_TOPIC "/slots/%d/command", &slotId) == 1) {
        // Handle command message
        if (msg == "open_gate") {
            openGateServo(slotId);
            mqtt_publish_ack(slotId, "gate_opened");
        } else if (msg == "close_gate") {
            closeGateServo(slotId);
            mqtt_publish_ack(slotId, "gate_closed");
        }
        // Add more commands as needed
    }
}

// Keep MQTT connection alive
void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqtt_connect();
    }
    mqttClient.loop();
}
