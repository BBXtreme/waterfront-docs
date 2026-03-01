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

// MQTT reconnect counter for telemetry
static int mqttReconnectCount = 0;

// Last MQTT activity timestamp for idle detection
unsigned long lastMqttActivity = 0;

// MQTT callback: Processes incoming messages and updates activity timestamp.
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    lastMqttActivity = millis();  // Track last activity for power management
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    ESP_LOGI("MQTT", "Message received: %s", message.c_str());

    // Parse JSON payload for commands or updates
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        ESP_LOGI("MQTT", "JSON parse failed");
        return;
    }
    // Future: Handle specific commands like config updates or gate control
}

// mqtt_init: Initializes MQTT connection with TLS if configured.
// Loads certs from LittleFS, sets up client, and connects with credentials.
esp_err_t mqtt_init() {
    // Retrieve broker and port from config with fallbacks
    String broker = g_config.mqtt.broker.length() > 0 ? g_config.mqtt.broker : "192.168.178.50";
    int port = g_config.mqtt.port > 0 ? g_config.mqtt.port : 1883;
    bool useTLS = g_config.mqtt.useTLS;

    bool tlsEnabled = useTLS;
    String caStr, certStr, keyStr;
    bool caLoaded = false, certLoaded = false;
    if (tlsEnabled) {
        // Load CA cert from LittleFS
        File ca = LittleFS.open(g_config.mqtt.caCertPath, "r");
        if (!ca) {
            ESP_LOGE("MQTT", "CA cert file missing – aborting TLS");
            tlsEnabled = false;
        } else {
            caStr = ca.readString();
            ca.close();  // Close file to free resources
            if (caStr.length() == 0) {
                ESP_LOGE("MQTT", "CA cert empty – aborting TLS");
                tlsEnabled = false;
            } else {
                caLoaded = true;
                // Load client cert/key if paths provided
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

    // Configure TLS if enabled
    if (tlsEnabled) {
        mqttClient.setCACert(caStr.c_str());
        if (certLoaded) {
            mqttClient.setCertificate(certStr.c_str(), keyStr.c_str());
        }
    }

    // Fallback to plain MQTT if TLS setup failed
    if (!tlsEnabled && port == 8883) port = 1883;

    // Set server and callback
    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(mqtt_callback);

    // Generate client ID
    String clientId = (g_config.mqtt.clientIdPrefix.length() > 0 ? g_config.mqtt.clientIdPrefix : "waterfront") + "-client";

    // Attempt connection with or without auth
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

// mqtt_publish_status: Publishes machine status to MQTT.
// Includes basic state info for monitoring.
void mqtt_publish_status() {
    DynamicJsonDocument doc(256);
    doc["state"] = "idle";
    doc["battery"] = 92;  // Placeholder, integrate with ADC
    doc["connType"] = "wifi";
    String payload;
    serializeJson(doc, payload);
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", locationCode.c_str());
    mqttClient.publish(topic, payload.c_str(), true);  // Retained for status
}

// mqtt_publish_slot_status: Publishes slot-specific status.
// Used for individual compartment updates.
void mqtt_publish_slot_status(int slotId, const char* jsonPayload) {
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    snprintf(topic, sizeof(topic), "waterfront/slots/%d/status", slotId);
    mqttClient.publish(topic, jsonPayload, true);  // Retained
}

// mqtt_loop_task: Maintains MQTT connection in a separate task.
// Reconnects on failure and increments reconnect counter.
void mqtt_loop_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        if (!mqttClient.connected()) {
            if (mqtt_init() != ESP_OK) {
                fatal_error("MQTT reconnect failed");  // Fatal if reconnect fails
            } else {
                mqttReconnectCount++;  // Track reconnects for telemetry
            }
        }
        mqttClient.loop();  // Process incoming messages
        vTaskDelay(pdMS_TO_TICKS(1000));  // Loop every second
    }
}

// getMqttReconnectCount: Returns reconnect count for telemetry.
int getMqttReconnectCount() {
    return mqttReconnectCount;
}
