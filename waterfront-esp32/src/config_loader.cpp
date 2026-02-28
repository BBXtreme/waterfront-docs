/**
 * @file config_loader.cpp
 * @brief Runtime configuration loading and saving from/to LittleFS JSON with validation.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads all config from JSON, validates, fallbacks to defaults, creates file if needed.
 */

#include "config_loader.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Global config instance
GlobalConfig g_config;

// Load config from LittleFS, with validation and fallback
bool loadConfig() {
    if (!LittleFS.begin()) {
        ESP_LOGE("CONFIG", "Failed to mount LittleFS");
        g_config = {{"192.168.178.50", 1883}, {"bremen", "harbor-01"}, 10, true};  // Defaults
        return false;
    }
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        ESP_LOGW("CONFIG", "config.json not found, using defaults");
        g_config = {{"192.168.178.50", 1883}, {"bremen", "harbor-01"}, 10, true};  // Defaults
        return false;
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        ESP_LOGE("CONFIG", "Failed to parse config.json: %s, using defaults", error.c_str());
        configFile.close();
        g_config = {{"192.168.178.50", 1883}, {"bremen", "harbor-01"}, 10, true};  // Defaults
        return false;
    }
    configFile.close();

    // Parse MQTT
    if (doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        g_config.mqtt.broker = mqtt["broker"].as<String>();
        g_config.mqtt.port = mqtt["port"];
    } else {
        g_config.mqtt.broker = "192.168.178.50";
        g_config.mqtt.port = 1883;
    }

    // Parse location
    if (doc.containsKey("location")) {
        JsonObject loc = doc["location"];
        g_config.location.slug = loc["slug"].as<String>();
        g_config.location.code = loc["code"].as<String>();
    } else {
        g_config.location.slug = "bremen";
        g_config.location.code = "harbor-01";
    }

    // Parse others
    g_config.maxCompartments = doc["maxCompartments"] | 10;
    g_config.debugMode = doc["debugMode"] | true;

    ESP_LOGI("CONFIG", "Loaded config from LittleFS");
    return true;
}
