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
// Edge cases: null topic/payload, invalid JSON, unknown commands.
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    lastMqttActivity = millis();  // Track last activity for power management
    if (!topic || !payload || length == 0) {
        ESP_LOGW("MQTT", "Invalid callback parameters: topic=%p, payload=%p, length=%u", topic, payload, length);
        return;
    }
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    ESP_LOGI("MQTT", "Message received on %s: %s", topic, message.c_str());

    // Parse JSON payload for commands or updates
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        ESP_LOGW("MQTT", "JSON parse failed: %s", error.c_str());
        return;
    }
    // Future: Handle specific commands like config updates or gate control
    // For now, just log
    ESP_LOGI("MQTT", "Parsed JSON successfully");
}

// Parse broker URI to extract host, port, and TLS flag
// Supports mqtt://host:port and mqtts://host:port
// Defaults to mqtts:// if no scheme
void parseBrokerUri(const String& brokerUri, String& host, int& port, bool& useTLS) {
    if (brokerUri.startsWith("mqtts://")) {
        useTLS = true;
        port = 8883;  // Default for MQTT over TLS
        host = brokerUri.substring(8);  // Remove "mqtts://"
    } else if (brokerUri.startsWith("mqtt://")) {
        useTLS = false;
        port = 1883;  // Default for plain MQTT
        host = brokerUri.substring(7);  // Remove "mqtt://"
    } else {
        // No scheme, default to mqtts://
        useTLS = true;
        port = 8883;
        host = "mqtts://" + brokerUri;
    }
    // Extract port if specified (e.g., host:port)
    int colonIndex = host.indexOf(':');
    if (colonIndex != -1) {
        port = host.substring(colonIndex + 1).toInt();
        host = host.substring(0, colonIndex);
    }
}

// mqtt_init: Initializes MQTT connection with TLS if configured.
// Loads certs from LittleFS, sets up client, and connects with credentials.
// Edge cases: missing certs, invalid broker, connection failures.
esp_err_t mqtt_init() {
    // Parse broker URI
    String host;
    int port;
    bool useTLS;
    parseBrokerUri(g_config.mqtt.broker, host, port, useTLS);
    ESP_LOGI("MQTT", "Parsed broker: host=%s, port=%d, useTLS=%d", host.c_str(), port, useTLS);

    // Override config useTLS if URI specifies
    if (useTLS) {
        g_config.mqtt.useTLS = true;
    }

    bool tlsEnabled = g_config.mqtt.useTLS;
    String caStr, certStr, keyStr;
    bool caLoaded = false, certLoaded = false;
    if (tlsEnabled) {
        // Check CA cert path
        if (g_config.mqtt.caCertPath.length() == 0) {
            ESP_LOGE("MQTT", "CA cert path empty – skipping TLS");
            tlsEnabled = false;
        } else {
            ESP_LOGI("MQTT", "Attempting to load CA cert from %s", g_config.mqtt.caCertPath.c_str());
            // Load CA cert from LittleFS
            File ca = LittleFS.open(g_config.mqtt.caCertPath, "r");
            if (!ca) {
                ESP_LOGE("MQTT", "CA cert file missing – skipping TLS");
                tlsEnabled = false;
            } else {
                caStr = ca.readString();
                ca.close();  // Close file to free resources
                if (caStr.length() == 0) {
                    ESP_LOGE("MQTT", "CA cert empty – skipping TLS");
                    tlsEnabled = false;
                } else {
                    caLoaded = true;
                    ESP_LOGI("MQTT", "Loaded CA cert, size=%d", caStr.length());
                    // Load client cert/key if paths provided
                    if (g_config.mqtt.clientCertPath.length() > 0) {
                        ESP_LOGI("MQTT", "Attempting to load client cert from %s", g_config.mqtt.clientCertPath.c_str());
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
                                ESP_LOGI("MQTT", "Loaded client cert/key, cert size=%d, key size=%d", certStr.length(), keyStr.length());
                            }
                        }
                    } else {
                        ESP_LOGI("MQTT", "Client cert path empty – using CA only");
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
        // Skip verification if configured (for testing, false in prod)
        if (g_config.mqtt.tlsSkipVerify) {
            mqttClient.setInsecure();  // Skip certificate verification
            ESP_LOGW("MQTT", "TLS certificate verification skipped (tlsSkipVerify=true)");
        }
        ESP_LOGI("MQTT", "TLS configured");
    }

    // Fallback to plain MQTT if TLS setup failed
    if (!tlsEnabled && port == 8883) {
        port = 1883;
        ESP_LOGW("MQTT", "TLS disabled, falling back to port 1883");
    }

    // Set server and callback
    mqttClient.setServer(host.c_str(), port);
    mqttClient.setCallback(mqtt_callback);
    ESP_LOGI("MQTT", "Server set to %s:%d", host.c_str(), port);

    // Generate client ID
    String clientId = (g_config.mqtt.clientIdPrefix.length() > 0 ? g_config.mqtt.clientIdPrefix : "waterfront") + "-client";
    if (clientId.length() > 23) {  // MQTT client ID max 23 chars
        clientId = clientId.substring(0, 23);
        ESP_LOGW("MQTT", "Client ID truncated to %s", clientId.c_str());
    }

    // Attempt connection with or without auth
    bool connected;
    if (g_config.mqtt.username.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), g_config.mqtt.username.c_str(), g_config.mqtt.password.c_str());
        ESP_LOGI("MQTT", "Connecting with auth: user=%s", g_config.mqtt.username.c_str());
    } else {
        connected = mqttClient.connect(clientId.c_str());
        ESP_LOGI("MQTT", "Connecting without auth");
    }
    if (connected) {
        ESP_LOGI("MQTT", "Connected and subscribed");
        return ESP_OK;
    } else {
        ESP_LOGE("MQTT", "Failed to connect, state=%d", mqttClient.state());
        return ESP_FAIL;
    }
}

// mqtt_publish_status: Publishes machine status to MQTT.
// Includes basic state info for monitoring.
// Edge cases: MQTT not connected, invalid topic.
void mqtt_publish_status() {
    if (!mqttClient.connected()) {
        ESP_LOGW("MQTT", "Not connected, skipping status publish");
        return;
    }
    DynamicJsonDocument doc(256);
    doc["state"] = "idle";
    doc["battery"] = 92;  // Placeholder, integrate with ADC
    doc["connType"] = "wifi";
    String payload;
    serializeJson(doc, payload);
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    int len = snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", locationCode.c_str());
    if (len >= sizeof(topic)) {
        ESP_LOGE("MQTT", "Status topic too long, skipping publish");
        return;
    }
    mqttClient.publish(topic, payload.c_str(), true);  // Retained for status
    ESP_LOGI("MQTT", "Published status to %s", topic);
}

// mqtt_publish_slot_status: Publishes slot-specific status.
// Used for individual compartment updates.
// Edge cases: invalid slotId, MQTT not connected.
void mqtt_publish_slot_status(int slotId, const char* jsonPayload) {
    if (!mqttClient.connected()) {
        ESP_LOGW("MQTT", "Not connected, skipping slot status publish");
        return;
    }
    if (slotId < 1 || !jsonPayload) {
        ESP_LOGE("MQTT", "Invalid slotId or payload for slot status");
        return;
    }
    char topic[64];
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    int len = snprintf(topic, sizeof(topic), "waterfront/slots/%d/status", slotId);
    if (len >= sizeof(topic)) {
        ESP_LOGE("MQTT", "Slot status topic too long, skipping publish");
        return;
    }
    mqttClient.publish(topic, jsonPayload, true);  // Retained
    ESP_LOGI("MQTT", "Published slot %d status to %s", slotId, topic);
}

// mqtt_loop_task: Maintains MQTT connection in a separate task.
// Reconnects on failure and increments reconnect counter.
// Edge cases: reconnect failures, watchdog resets.
void mqtt_loop_task(void *pvParameters) {
    // Add this task to watchdog for monitoring
    esp_task_wdt_add(NULL);
    ESP_LOGI("MQTT", "MQTT loop task started");
    while (1) {
        esp_task_wdt_reset();  // Reset watchdog
        if (!mqttClient.connected()) {
            ESP_LOGI("MQTT", "Disconnected, attempting reconnect");
            if (mqtt_init() != ESP_OK) {
                ESP_LOGE("MQTT", "Reconnect failed, will retry");
                fatal_error("MQTT reconnect failed");  // Fatal if reconnect fails
            } else {
                mqttReconnectCount++;  // Track reconnects for telemetry
                ESP_LOGI("MQTT", "Reconnected successfully, count=%d", mqttReconnectCount);
            }
        }
        mqttClient.loop();  // Process incoming messages
        vTaskDelay(pdMS_TO_TICKS(1000));  // Loop every second
    }
}

// getMqttReconnectCount: Returns reconnect count for telemetry.
// Edge case: count overflow (int max), but unlikely.
int getMqttReconnectCount() {
    return mqttReconnectCount;
}
