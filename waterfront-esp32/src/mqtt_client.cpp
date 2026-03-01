#include "mqtt_client.h"
#include "config_loader.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "error_handler.h"
#include <LittleFS.h>

// Adapted from original mqtt_event_handler in mdb-slave-esp32s3.c
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// Extern global config
extern GlobalConfig g_config;

// MQTT reconnect counter
static int mqttReconnectCount = 0;

// Last MQTT activity timestamp
unsigned long lastMqttActivity = 0;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    lastMqttActivity = millis();  // Update activity timestamp
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    ESP_LOGI("MQTT", "Message received: %s", message.c_str());

    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        ESP_LOGI("MQTT", "JSON parse failed");
        return;
    }
}

esp_err_t mqtt_init() {
    // Use runtime config with fallback
    String broker = g_config.mqtt.broker.length() > 0 ? g_config.mqtt.broker : "192.168.178.50";
    int port = g_config.mqtt.port > 0 ? g_config.mqtt.port : 1883;
    bool useTLS = g_config.mqtt.useTLS;

    bool tlsEnabled = useTLS;
    String caStr, certStr, keyStr;
    bool caLoaded = false, certLoaded = false;
    if (tlsEnabled) {
        File ca = LittleFS.open(g_config.mqtt.caCertPath, "r");
        if (!ca) {
            ESP_LOGE("MQTT", "CA cert file missing – aborting TLS");
            tlsEnabled = false;
        } else {
            caStr = ca.readString();
            ca.close();  // Ensure file is closed
            if (caStr.length() == 0) {
                ESP_LOGE("MQTT", "CA cert empty – aborting TLS");
                tlsEnabled = false;
            } else {
                caLoaded = true;
                if (g_config.mqtt.clientCertPath.length() > 0) {
                    File cert = LittleFS.open(g_config.mqtt.clientCertPath, "r");
                    File key = LittleFS.open(g_config.mqtt.clientKeyPath, "r");
                    if (!cert || !key) {
                        ESP_LOGE("MQTT", "Client cert/key file missing – continuing without client cert");
                        cert.close();
                        key.close();
                    } else {
                        certStr = cert.readString();
                        keyStr = key.readString();
                        cert.close();
                        key.close();
                        if (certStr.length() == 0 || keyStr.length() == 0) {
                            ESP_LOGE("MQTT", "Client cert/key empty – continuing without client cert");
                        } else {
                            certLoaded = true;
                        }
                    }
                }
            }
        }
    }

    if (tlsEnabled) {
        mqttClient.setCACert(caStr.c_str());
        if (certLoaded) {
            mqttClient.setCertificate(certStr.c_str(), keyStr.c_str());
        }
    }

    if (!tlsEnabled && port == 8883) port = 1883;

    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(mqtt_callback);
    String clientId = (g_config.mqtt.clientIdPrefix.length() > 0 ? g_config.mqtt.clientIdPrefix : "waterfront") + "-client";
    bool connected;
    if (g_config.mqtt.username.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), g_config.mqtt.username.c_str(), g_config.mqtt.password.c_str());
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    if (connected) {
        ESP_LOGI("MQTT", "Connected and subscribed");
        return ESP_OK;
    } else {
        ESP_LOGE("MQTT", "Failed to connect");
        return ESP_FAIL;
    }
}

void mqtt_publish_status() {
    DynamicJsonDocument doc(256);
    doc["state"] = "idle";
    doc["battery"] = 92;
    doc["connType"] = "wifi";
    String payload;
    serializeJson(doc, payload);
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", locationCode.c_str());
    mqttClient.publish(topic, payload.c_str(), true);  // Retained publish for machine status
}

// New function for publishing slot-specific status (e.g., booked, gateState)
void mqtt_publish_slot_status(int slotId, const char* jsonPayload) {
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    snprintf(topic, sizeof(topic), "waterfront/slots/%d/status", slotId);
    mqttClient.publish(topic, jsonPayload, true);  // Retained publish for slot status
}

// MQTT loop task with watchdog reset
void mqtt_loop_task(void *pvParameters) {
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog at start of loop
        if (!mqttClient.connected()) {
            if (mqtt_init() != ESP_OK) {
                fatal_error("MQTT reconnect failed");
            } else {
                mqttReconnectCount++;  // Increment on successful reconnect
            }
        }
        mqttClient.loop();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Function to get MQTT reconnect count
int getMqttReconnectCount() {
    return mqttReconnectCount;
}
