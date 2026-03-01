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
// Added OTA update support.

#include "mqtt_handler.h"
#include "mqtt_topics.h"
#include "gate_control.h"
#include "config_loader.h"
#include "deposit_logic.h"
#include <PubSubClient.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <HTTPUpdate.h>
#include <Preferences.h>

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

// Last-will topic and message for disconnect handling
#define MQTT_LAST_WILL_TOPIC "waterfront/esp32/disconnect"
#define MQTT_LAST_WILL_MESSAGE "{\"status\":\"disconnected\"}"

// Initialize MQTT handler
void mqtt_init() {
    vPortEnterCritical(&g_configMutex);
    String broker = g_config.mqtt.broker;
    int port = g_config.mqtt.port;
    vPortExitCritical(&g_configMutex);
    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(mqtt_callback);
    gate_init();  // Initialize gate control
    ESP_LOGI("MQTT", "Initialized with broker %s:%d", broker.c_str(), port);
}

// Attempt to connect to MQTT broker with last-will
bool mqtt_connect() {
    if (mqttClient.connected()) return true;
    ESP_LOGI("MQTT", "Connecting...");
    // Set last-will (PubSubClient syntax: connect(clientId, willTopic, willQoS, willRetain, willMessage))
    vPortEnterCritical(&g_configMutex);
    String clientIdPrefix = g_config.mqtt.clientIdPrefix;
    vPortExitCritical(&g_configMutex);
    if (mqttClient.connect((clientIdPrefix + "-client").c_str(), MQTT_LAST_WILL_TOPIC, 1, true, MQTT_LAST_WILL_MESSAGE)) {
        ESP_LOGI("MQTT", "Connected");
        mqtt_subscribe();
        return true;
    } else {
        ESP_LOGE("MQTT", "Failed, rc=%d", mqttClient.state());
        return false;
    }
}

// Subscribe to compartment-specific MQTT topics using wildcards
void mqtt_subscribe() {
    char topic[96];
    // Subscribe to status wildcard for this location
    vPortEnterCritical(&g_configMutex);
    String locationSlug = g_config.location.slug;
    String locationCode = g_config.location.code;
    vPortExitCritical(&g_configMutex);
    snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/+/status", locationSlug.c_str(), locationCode.c_str());
    mqttClient.subscribe(topic, 1);
    ESP_LOGI("MQTT", "Subscribed to %s", topic);
    // Subscribe to command wildcard for this location
    snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/+/command", locationSlug.c_str(), locationCode.c_str());
    mqttClient.subscribe(topic, 1);
    ESP_LOGI("MQTT", "Subscribed to %s", topic);
    // Subscribe to config update
    std::string configTopic = "waterfront/" + locationSlug + "/" + locationCode + "/config/update";
    mqttClient.subscribe(configTopic.c_str(), 1);
    ESP_LOGI("MQTT", "Subscribed to %s", configTopic.c_str());
    // Subscribe to OTA update
    std::string otaTopic = "waterfront/" + locationSlug + "/" + locationCode + "/ota/update";
    mqttClient.subscribe(otaTopic.c_str(), 1);
    ESP_LOGI("MQTT", "Subscribed to %s", otaTopic.c_str());
    // Subscribe to booking paid
    std::string bookingPaidTopic = "waterfront/" + locationSlug + "/" + locationCode + "/booking/paid";
    mqttClient.subscribe(bookingPaidTopic.c_str(), 1);
    ESP_LOGI("MQTT", "Subscribed to %s", bookingPaidTopic.c_str());
}

// Publish retained status for a compartment with CRC32
void mqtt_publish_retained_status(int compartmentId, const char* jsonPayload) {
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, jsonPayload);
    if (error) {
        ESP_LOGE("MQTT", "Failed to parse JSON for status publish: %s", error.c_str());
        return;  // Discard bad message
    }
    uint32_t crc = computeCRC32(jsonPayload, strlen(jsonPayload));
    doc["crc"] = crc;
    doc["firmwareVersion"] = FW_VERSION;  // Include firmware version
    String updatedPayload;
    serializeJson(doc, updatedPayload);
    char topic[96];
    vPortEnterCritical(&g_configMutex);
    String locationSlug = g_config.location.slug;
    String locationCode = g_config.location.code;
    vPortExitCritical(&g_configMutex);
    snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/%d/status", locationSlug.c_str(), locationCode.c_str(), compartmentId);
    mqttClient.publish(topic, updatedPayload.c_str(), true);
    ESP_LOGI("MQTT", "Published retained status to %s: %s", topic, updatedPayload.c_str());
}

// Publish acknowledgment for a compartment action with CRC32
void mqtt_publish_ack(int compartmentId, const char* action) {
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"compartmentId\":%d,\"action\":\"%s\",\"timestamp\":%lu}", compartmentId, action, millis());
    uint32_t crc = computeCRC32(payload, strlen(payload));
    char fullPayload[256];
    snprintf(fullPayload, sizeof(fullPayload), "%s,\"crc\":%lu}", payload, crc);
    fullPayload[strlen(fullPayload) - 1] = '}';  // Fix JSON
    char topic[96];
    vPortEnterCritical(&g_configMutex);
    String locationSlug = g_config.location.slug;
    String locationCode = g_config.location.code;
    vPortExitCritical(&g_configMutex);
    snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/%d/ack", locationSlug.c_str(), locationCode.c_str(), compartmentId);
    mqttClient.publish(topic, fullPayload, true);
    ESP_LOGI("MQTT", "Published ack to %s: %s", topic, fullPayload);
}

// MQTT callback for processing incoming messages with CRC validation
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String msg;
    for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
    ESP_LOGI("MQTT", "Received on %s: %s at %lu", topic, msg.c_str(), millis());

    // Check if config update
    vPortEnterCritical(&g_configMutex);
    String locationSlug = g_config.location.slug;
    String locationCode = g_config.location.code;
    vPortExitCritical(&g_configMutex);
    std::string configTopic = "waterfront/" + locationSlug + "/" + locationCode + "/config/update";
    if (strcmp(topic, configTopic.c_str()) == 0) {
        ESP_LOGI("MQTT", "Config update received");
        if (updateConfigFromJson(msg.c_str())) {
            ESP_LOGI("MQTT", "Config updated, restarting ESP");
            delay(5000);
            esp_restart();
        } else {
            ESP_LOGE("MQTT", "Failed to update config");
        }
        return;
    }

    // Check if OTA update
    std::string otaTopic = "waterfront/" + locationSlug + "/" + locationCode + "/ota/update";
    if (strcmp(topic, otaTopic.c_str()) == 0) {
        ESP_LOGI("MQTT", "OTA update received: %s", msg.c_str());
        // Save current version to NVS before update
        Preferences prefs;
        prefs.begin("ota", false);
        prefs.putString("prev_version", FW_VERSION);
        prefs.putBool("attempted", true);
        prefs.end();
        // Assume msg is the URL
        t_httpUpdate_return ret = httpUpdate.update(msg);
        DynamicJsonDocument otaDoc(256);
        otaDoc["firmwareVersion"] = FW_VERSION;
        switch (ret) {
            case HTTP_UPDATE_FAILED:
                ESP_LOGE("OTA", "Update failed: %s", httpUpdate.getLastErrorString().c_str());
                otaDoc["otaResult"] = "failed";
                otaDoc["error"] = httpUpdate.getLastErrorString().c_str();
                break;
            case HTTP_UPDATE_NO_UPDATES:
                ESP_LOGI("OTA", "No updates available");
                otaDoc["otaResult"] = "no_updates";
                break;
            case HTTP_UPDATE_OK:
                ESP_LOGI("OTA", "Update successful, restarting");
                otaDoc["otaResult"] = "success";
                break;
            default:
                ESP_LOGE("OTA", "Unknown update result: %d", ret);
                otaDoc["otaResult"] = "unknown";
                break;
        }
        String otaPayload;
        serializeJson(otaDoc, otaPayload);
        std::string otaStatusTopic = "waterfront/" + locationSlug + "/" + locationCode + "/ota/status";
        mqttClient.publish(otaStatusTopic.c_str(), otaPayload.c_str(), true);
        if (ret == HTTP_UPDATE_OK) {
            ESP.restart();
        }
        return;
    }

    // Check if booking paid
    std::string bookingPaidTopic = "waterfront/" + locationSlug + "/" + locationCode + "/booking/paid";
    if (strcmp(topic, bookingPaidTopic.c_str()) == 0) {
        ESP_LOGI("MQTT", "Booking paid received: %s", msg.c_str());
        DynamicJsonDocument doc(256);
        DeserializationError error = deserializeJson(doc, msg);
        if (error) {
            ESP_LOGE("MQTT", "Failed to parse booking paid payload");
            return;
        }
        String bookingId = doc["bookingId"];
        int compartmentId = doc["compartmentId"];
        unsigned long durationSec = doc["durationSec"];
        // Start rental timer
        vPortEnterCritical(&g_configMutex);
        unsigned long gracePeriod = g_config.system.gracePeriodSec;
        vPortExitCritical(&g_configMutex);
        startRental(compartmentId, durationSec + gracePeriod);
        ESP_LOGI("MQTT", "Started rental for booking %s, compartment %d, duration %lu sec", bookingId.c_str(), compartmentId, durationSec);
        return;
    }

    // Validate CRC for other messages
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        ESP_LOGE("MQTT", "JSON parse failed for status: %s", error.c_str());
        return;  // Discard bad message
    }
    uint32_t receivedCrc = doc["crc"];
    doc.remove("crc");  // Remove for recompute
    String cleanMsg;
    serializeJson(doc, cleanMsg);
    uint32_t computedCrc = computeCRC32(cleanMsg.c_str(), cleanMsg.length());
    if (receivedCrc != computedCrc) {
        ESP_LOGE("MQTT", "CRC mismatch: received %lu, computed %lu", receivedCrc, computedCrc);
        return;  // Discard bad message
    }

    // Log full JSON for debugging
    ESP_LOGV("MQTT", "Full JSON after CRC validation: %s", cleanMsg.c_str());

    // Parse topic to extract compartmentId using sscanf for efficiency
    int compartmentId = -1;
    char locationSlugBuf[32], locationCodeBuf[32];
    if (sscanf(topic, "waterfront/%31[^/]/%31[^/]/compartments/%d/status", locationSlugBuf, locationCodeBuf, &compartmentId) == 3) {
        // Handle status message (retained sync)
        bool booked = doc["booked"];
        const char* gateState = doc["gateState"];
        ESP_LOGI("MQTT", "Compartment %d: booked=%d, gateState=%s", compartmentId, booked, gateState);
        if (booked && strcmp(gateState, "locked") == 0) {
            openCompartmentGate(compartmentId);
            mqtt_publish_ack(compartmentId, "gate_opened");
        }
    } else if (sscanf(topic, "waterfront/%31[^/]/%31[^/]/compartments/%d/command", locationSlugBuf, locationCodeBuf, &compartmentId) == 3) {
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
            vPortEnterCritical(&g_configMutex);
            String locSlug = g_config.location.slug;
            String locCode = g_config.location.code;
            vPortExitCritical(&g_configMutex);
            snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/%d/ack", locSlug.c_str(), locCode.c_str(), compartmentId);
            mqttClient.publish(topic, fullPayload, true);
        }
    }
}

// Maintain MQTT connection and process messages
void mqtt_loop() {
    if (!mqttClient.connected()) {
        mqtt_connect();
    }
    mqttClient.loop();
}
