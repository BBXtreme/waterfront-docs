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
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config_loader.h"

// Extern MQTT client for publishing alerts
extern PubSubClient mqttClient;

// fatal_error function: Handles unrecoverable errors.
// Logs the error, publishes to MQTT if connected, and restarts the ESP32.
// Edge cases: MQTT not connected (skips publish), invalid code (logs as is).
void fatal_error(const char* msg, esp_err_t code) {
    // Validate inputs
    if (!msg) {
        msg = "Unknown error";  // Fallback for null message
    }
    // Log error with code name for debugging
    ESP_LOGE("FATAL", "%s: %s (0x%x)", msg, esp_err_to_name(code), code);

    // Publish fatal error to debug topic if MQTT connected and debug mode enabled
    if (mqttClient.connected() && g_config.debugMode) {
        DynamicJsonDocument doc(256);
        doc["error"] = msg;
        doc["code"] = esp_err_to_name(code);  // Human-readable error code
        doc["timestamp"] = millis();
        String payload;
        serializeJson(doc, payload);
        char topic[96];
        int len = snprintf(topic, sizeof(topic), "waterfront/%s/%s/debug", g_config.location.slug.c_str(), g_config.location.code.c_str());
        if (len >= sizeof(topic)) {
            ESP_LOGE("FATAL", "Topic too long for buffer, skipping debug publish");
        } else {
            mqttClient.publish(topic, payload.c_str(), false);  // Not retained for debug
            ESP_LOGI("FATAL", "Published fatal error to debug topic");
        }
    } else if (!mqttClient.connected()) {
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
    int alertLen = snprintf(alertTopic, sizeof(alertTopic), "waterfront/%s/%s/alert", g_config.location.slug.c_str(), g_config.location.code.c_str());
    if (alertLen >= sizeof(alertTopic)) {
        ESP_LOGE("FATAL", "Alert topic too long for buffer, skipping alert publish");
    } else {
        mqttClient.publish(alertTopic, alertPayload.c_str(), false);  // Not retained
        ESP_LOGI("FATAL", "Published alert to %s", alertTopic);
    }

    // Delay to allow publish to complete before restart
    delay(5000);
    esp_restart();  // Restart ESP32 to recover
}
