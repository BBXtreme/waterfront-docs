#include "mqtt_client.h"
#include "config_loader.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include "error_handler.h"
#include <LittleFS.h>

// ESP-IDF MQTT client handle
esp_mqtt_client_handle_t mqttClient = nullptr;

// MQTT connection state
bool mqttConnected = false;

// MQTT reconnect counter for telemetry
static int mqttReconnectCount = 0;

// Last MQTT activity timestamp for idle detection
unsigned long lastMqttActivity = 0;

// Mutex for thread-safe access to MQTT state
portMUX_TYPE mqttMutex = portMUX_INITIALIZER_UNLOCKED;

// MQTT event handler: Processes incoming events and updates activity timestamp.
// Handles connect, disconnect, data, etc.
// Edge cases: invalid data, unknown events.
void event_handler(void* args, esp_event_base_t base, int32_t event_id, void* data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) data;
    switch (event_id) {
        case MQTT_EVENT_CONNECTED:
            vPortEnterCritical(&mqttMutex);
            mqttConnected = true;
            vPortExitCritical(&mqttMutex);
            ESP_LOGI("MQTT", "Connected to MQTT broker");
            break;
        case MQTT_EVENT_DISCONNECTED:
            vPortEnterCritical(&mqttMutex);
            mqttConnected = false;
            vPortExitCritical(&mqttMutex);
            ESP_LOGW("MQTT", "Disconnected from MQTT broker");
            break;
        case MQTT_EVENT_DATA:
            vPortEnterCritical(&mqttMutex);
            lastMqttActivity = millis();  // Track last activity for power management
            vPortExitCritical(&mqttMutex);
            if (!event->topic || !event->data || event->data_len == 0) {
                ESP_LOGW("MQTT", "Invalid event data: topic=%p, data=%p, len=%d", event->topic, event->data, event->data_len);
                return;
            }
            String message = "";
            for (int i = 0; i < event->data_len; i++) {
                message += (char)event->data[i];
            }
            ESP_LOGI("MQTT", "Message received on %.*s: %s", event->topic_len, event->topic, message.c_str());

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
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE("MQTT", "MQTT error occurred");
            break;
        default:
            ESP_LOGD("MQTT", "Unhandled MQTT event: %d", event_id);
            break;
    }
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
    vPortEnterCritical(&g_configMutex);
    String brokerUri = g_config.mqtt.broker;
    vPortExitCritical(&g_configMutex);
    String host;
    int port;
    bool useTLS;
    parseBrokerUri(brokerUri, host, port, useTLS);
    ESP_LOGI("MQTT", "Parsed broker: host=%s, port=%d, useTLS=%d", host.c_str(), port, useTLS);

    // Override config useTLS if URI specifies
    if (useTLS) {
        vPortEnterCritical(&g_configMutex);
        g_config.mqtt.useTLS = true;
        vPortExitCritical(&g_configMutex);
    }

    // Build full URI
    String fullUri = (useTLS ? "mqtts://" : "mqtt://") + host + ":" + String(port);

    // Configure MQTT client
    esp_mqtt_client_config_t mqtt_config = {};
    mqtt_config.uri = fullUri.c_str();

    // Generate client ID
    vPortEnterCritical(&g_configMutex);
    String clientIdPrefix = g_config.mqtt.clientIdPrefix;
    vPortExitCritical(&g_configMutex);
    String clientId = (clientIdPrefix.length() > 0 ? clientIdPrefix : "waterfront") + "-client";
    if (clientId.length() > 23) {  // MQTT client ID max 23 chars
        clientId = clientId.substring(0, 23);
        ESP_LOGW("MQTT", "Client ID truncated to %s", clientId.c_str());
    }
    mqtt_config.client_id = clientId.c_str();

    // Set auth if provided
    vPortEnterCritical(&g_configMutex);
    String username = g_config.mqtt.username;
    String password = g_config.mqtt.password;
    vPortExitCritical(&g_configMutex);
    if (username.length() > 0) {
        mqtt_config.username = username.c_str();
        mqtt_config.password = password.c_str();
        ESP_LOGI("MQTT", "Using auth: user=%s", username.c_str());
    }

    // Configure TLS if enabled
    bool tlsEnabled = useTLS;
    if (tlsEnabled) {
        // Check CA cert path
        vPortEnterCritical(&g_configMutex);
        String caCertPath = g_config.mqtt.caCertPath;
        vPortExitCritical(&g_configMutex);
        if (caCertPath.length() == 0) {
            ESP_LOGE("MQTT", "CA cert path empty – skipping TLS");
            tlsEnabled = false;
        } else {
            ESP_LOGI("MQTT", "Attempting to load CA cert from %s", caCertPath.c_str());
            // Load CA cert from LittleFS
            File ca = LittleFS.open(caCertPath, "r");
            if (!ca) {
                ESP_LOGE("MQTT", "CA cert file missing – skipping TLS");
                tlsEnabled = false;
            } else {
                String caStr = ca.readString();
                ca.close();  // Close file to free resources
                if (caStr.length() == 0) {
                    ESP_LOGE("MQTT", "CA cert empty – skipping TLS");
                    tlsEnabled = false;
                } else {
                    mqtt_config.cert_pem = caStr.c_str();
                    ESP_LOGI("MQTT", "Loaded CA cert, size=%d", caStr.length());
                    // Load client cert/key if paths provided
                    vPortEnterCritical(&g_configMutex);
                    String clientCertPath = g_config.mqtt.clientCertPath;
                    String clientKeyPath = g_config.mqtt.clientKeyPath;
                    vPortExitCritical(&g_configMutex);
                    if (clientCertPath.length() > 0) {
                        ESP_LOGI("MQTT", "Attempting to load client cert from %s", clientCertPath.c_str());
                        File cert = LittleFS.open(clientCertPath, "r");
                        File key = LittleFS.open(clientKeyPath, "r");
                        if (!cert || !key) {
                            ESP_LOGE("MQTT", "Client cert/key file missing – continuing without client cert");
                            cert.close();
                            key.close();
                        } else {
                            String certStr = cert.readString();
                            String keyStr = key.readString();
                            cert.close();
                            key.close();
                            if (certStr.length() == 0 || keyStr.length() == 0) {
                                ESP_LOGE("MQTT", "Client cert/key empty – continuing without client cert");
                            } else {
                                mqtt_config.client_cert_pem = certStr.c_str();
                                mqtt_config.client_key_pem = keyStr.c_str();
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

    // Skip verification if configured (for testing, false in prod)
    vPortEnterCritical(&g_configMutex);
    bool tlsSkipVerify = g_config.mqtt.tlsSkipVerify;
    vPortExitCritical(&g_configMutex);
    if (tlsSkipVerify) {
        mqtt_config.skip_cert_common_name_check = true;
        ESP_LOGW("MQTT", "TLS certificate verification skipped (tlsSkipVerify=true)");
    }

    // Initialize and start MQTT client
    mqttClient = esp_mqtt_client_init(&mqtt_config);
    if (!mqttClient) {
        ESP_LOGE("MQTT", "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(mqttClient);
    if (err != ESP_OK) {
        ESP_LOGE("MQTT", "Failed to start MQTT client: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI("MQTT", "MQTT client initialized and started");
    return ESP_OK;
}

// mqtt_publish_status: Publishes machine status to MQTT.
// Includes basic state info for monitoring.
// Edge cases: MQTT not connected, invalid topic.
void mqtt_publish_status() {
    vPortEnterCritical(&mqttMutex);
    bool connected = mqttConnected;
    vPortExitCritical(&mqttMutex);
    if (!connected) {
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
    vPortEnterCritical(&g_configMutex);
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    vPortExitCritical(&g_configMutex);
    int len = snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", locationCode.c_str());
    if (len >= sizeof(topic)) {
        ESP_LOGE("MQTT", "Status topic too long, skipping publish");
        return;
    }
    int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload.c_str(), 0, 1, 1);  // QoS 1, retain
    if (msg_id < 0) {
        ESP_LOGE("MQTT", "Failed to publish status");
    } else {
        ESP_LOGI("MQTT", "Published status to %s, msg_id=%d", topic, msg_id);
    }
}

// mqtt_publish_slot_status: Publishes slot-specific status.
// Used for individual compartment updates.
// Edge cases: invalid slotId, MQTT not connected.
void mqtt_publish_slot_status(int slotId, const char* jsonPayload) {
    vPortEnterCritical(&mqttMutex);
    bool connected = mqttConnected;
    vPortExitCritical(&mqttMutex);
    if (!connected) {
        ESP_LOGW("MQTT", "Not connected, skipping slot status publish");
        return;
    }
    if (slotId < 1 || !jsonPayload) {
        ESP_LOGE("MQTT", "Invalid slotId or payload for slot status");
        return;
    }
    char topic[64];
    vPortEnterCritical(&g_configMutex);
    String locationCode = g_config.location.code.length() > 0 ? g_config.location.code : "harbor-01";
    vPortExitCritical(&g_configMutex);
    int len = snprintf(topic, sizeof(topic), "waterfront/slots/%d/status", slotId);
    if (len >= sizeof(topic)) {
        ESP_LOGE("MQTT", "Slot status topic too long, skipping publish");
        return;
    }
    int msg_id = esp_mqtt_client_publish(mqttClient, topic, jsonPayload, 0, 1, 1);  // QoS 1, retain
    if (msg_id < 0) {
        ESP_LOGE("MQTT", "Failed to publish slot status");
    } else {
        ESP_LOGI("MQTT", "Published slot %d status to %s, msg_id=%d", slotId, topic, msg_id);
    }
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
        vPortEnterCritical(&mqttMutex);
        bool connected = mqttConnected;
        vPortExitCritical(&mqttMutex);
        if (!connected) {
            ESP_LOGI("MQTT", "Disconnected, attempting reconnect");
            if (mqtt_init() != ESP_OK) {
                ESP_LOGE("MQTT", "Reconnect failed, will retry");
                fatal_error("MQTT reconnect failed");  // Fatal if reconnect fails
            } else {
                vPortEnterCritical(&mqttMutex);
                mqttReconnectCount++;  // Track reconnects for telemetry
                vPortExitCritical(&mqttMutex);
                ESP_LOGI("MQTT", "Reconnected successfully, count=%d", mqttReconnectCount);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second
    }
}

// getMqttReconnectCount: Returns reconnect count for telemetry.
// Edge case: count overflow (int max), but unlikely.
int getMqttReconnectCount() {
    vPortEnterCritical(&mqttMutex);
    int count = mqttReconnectCount;
    vPortExitCritical(&mqttMutex);
    return count;
}
