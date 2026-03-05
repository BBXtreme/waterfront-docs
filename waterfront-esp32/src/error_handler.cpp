/**
 * @file error_handler.cpp
 * @brief Implementation of global error handling for WATERFRONT ESP32.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Logs fatal errors, publishes alerts to MQTT, and restarts ESP32.
 *       Handles edge cases like MQTT not connected, invalid codes.
 */

#include "error_handler.h"
#include <esp_log.h>
#include <esp_system.h>
#include <Arduino.h>
#include <mqtt_client.h>
#include <ArduinoJson.h>
#include "config_loader.h"

// Extern MQTT client for publishing alerts
extern esp_mqtt_client_handle_t mqttClient;
extern bool mqttConnected;

// fatal_error function: Handles unrecoverable errors.
// Logs the error, publishes to MQTT if connected and debug mode enabled, and restarts the ESP32.
// Edge cases: MQTT not connected (skips publish), invalid code (logs as is).
void fatal_error(const char* msg, esp_err_t code = ESP_FAIL) {
    // Validate inputs
    if (!msg) {
        msg = "Unknown error";  // Fallback for null message
    }
    // Log error with code name for debugging
    ESP_LOGE("FATAL", "%s: %s (0x%x)", msg, esp_err_to_name(code), code);

    // Publish fatal error to debug topic if MQTT connected and debug mode enabled
    vPortEnterCritical(&g_configMutex);
    bool debugEnabled = g_config.system.debugMode;
    vPortExitCritical(&g_configMutex);
    if (mqttConnected && debugEnabled) {
        DynamicJsonDocument doc(256);
        doc["error"] = msg;
        doc["code"] = esp_err_to_name(code);  // Human-readable error code
        doc["timestamp"] = millis();
        String payload;
        serializeJson(doc, payload);
        char topic[96];
        vPortEnterCritical(&g_configMutex);
        int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug, g_config.location.code);
        vPortExitCritical(&g_configMutex);
        if (len >= sizeof(topic)) {
            ESP_LOGE("FATAL", "Topic too long for buffer, skipping debug publish");
        } else {
            int msg_id = esp_mqtt_client_publish(mqttClient, topic, payload.c_str(), 0, 1, 0);  // QoS 1, no retain
            if (msg_id >= 0) {
                ESP_LOGI("FATAL", "Published fatal error to debug topic, msg_id=%d", msg_id);
            } else {
                ESP_LOGE("FATAL", "Failed to publish fatal error to debug topic");
            }
        }
    } else if (!mqttConnected) {
        ESP_LOGW("FATAL", "MQTT not connected, skipping debug publish");
    }

    // Publish to alert topic for critical alerts (always attempted)
    DynamicJsonDocument alertDoc(256);
    alertDoc["error"] = msg;
    alertDoc["code"] = code;  // Numeric code for alerts
    alertDoc["timestamp"] = millis();
    String alertPayload;
    serializeJson(alertDoc, alertPayload);
    char alertTopic[96];
    vPortEnterCritical(&g_configMutex);
    int alertLen = snprintf(alertTopic, sizeof(alertTopic), "waterfront/%s/%s/alert", g_config.location.slug, g_config.location.code);
    vPortExitCritical(&g_configMutex);
    if (alertLen >= sizeof(alertTopic)) {
        ESP_LOGE("FATAL", "Alert topic too long for buffer, skipping alert publish");
    } else {
        int msg_id = esp_mqtt_client_publish(mqttClient, alertTopic, alertPayload.c_str(), 0, 1, 0);  // QoS 1, no retain
        if (msg_id >= 0) {
            ESP_LOGI("FATAL", "Published alert to %s, msg_id=%d", alertTopic, msg_id);
        } else {
            ESP_LOGE("FATAL", "Failed to publish alert");
        }
    }

    // Delay to allow publish to complete before restart
    delay(5000);
    esp_restart();  // Restart ESP32 to recover
}
