// mqtt_handler.cpp - MQTT client handling for WATERFRONT ESP32
// This file manages MQTT connections, subscriptions, publishing, and callbacks.
// It integrates with the main loop for reliable IoT communication.
// Uses PubSubClient for Arduino compatibility.

#include "mqtt_handler.h"
#include "config.h"
#include "relay_handler.h"  // Added to declare relay_unlock()
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// External references
extern PubSubClient mqttClient;
extern WiFiClient wifiClient;
extern bool provisioningActive;

// MQTT topics (defined in config.h)
#define MQTT_UNLOCK_TOPIC "/kayak/" MACHINE_ID "/unlock"
#define MQTT_STATUS_TOPIC "/kayak/" MACHINE_ID "/status"
#define MQTT_PROVISION_TOPIC "/kayak/" MACHINE_ID "/provision/start"

// Initialize MQTT handler
void mqtt_init() {
    mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    ESP_LOGI("MQTT", "Initialized with broker %s:%d", MQTT_BROKER, MQTT_PORT);
}

// Connect to MQTT broker
bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    ESP_LOGI("MQTT", "Connecting...");
    if (mqttClient.connect(MACHINE_ID)) {
        ESP_LOGI("MQTT", "Connected");
        mqtt_subscribe();
        mqtt_publish_status();
        return true;
    } else {
        ESP_LOGE("MQTT", "Failed, rc=%d", mqttClient.state());
        return false;
    }
}

// Subscribe to topics
void mqtt_subscribe() {
    mqttClient.subscribe(MQTT_UNLOCK_TOPIC);
    mqttClient.subscribe(MQTT_PROVISION_TOPIC);
    ESP_LOGI("MQTT", "Subscribed to topics");
}

// Publish status
void mqtt_publish_status() {
    DynamicJsonDocument doc(256);
    doc["wifiState"] = WiFi.status() == WL_CONNECTED ? "connected" : "disconnected";
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(MQTT_STATUS_TOPIC, msg.c_str());
    ESP_LOGI("MQTT", "Published status");
}

// MQTT callback for incoming messages
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    ESP_LOGI("MQTT", "Received on %s: %s", topic, msg.c_str());

    if (strcmp(topic, MQTT_UNLOCK_TOPIC) == 0) {
        // Parse unlock command
        DynamicJsonDocument doc(256);
        deserializeJson(doc, msg);
        String pin = doc["pin"];
        if (pin == "1234") {  // TODO: Secure validation
            relay_unlock();
            ESP_LOGI("MQTT", "Unlock command processed");
        } else {
            ESP_LOGW("MQTT", "Invalid PIN");
        }
    } else if (strcmp(topic, MQTT_PROVISION_TOPIC) == 0) {
        provisioningActive = true;
        ESP_LOGI("MQTT", "Provisioning triggered");
    }
}

// Keep MQTT connection alive
void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqtt_connect();
    }
    mqttClient.loop();
}

// Publish event (e.g., sensor or deposit)
void mqtt_publish_event(const char* event, const char* details) {
    DynamicJsonDocument doc(256);
    doc["event"] = event;
    doc["details"] = details;
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish("/kayak/" MACHINE_ID "/event", msg.c_str());
    ESP_LOGI("MQTT", "Published event: %s", event);
}
