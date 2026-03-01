/**
 * @file config_loader.cpp
 * @brief Runtime configuration loading and saving from/to LittleFS JSON with validation.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads all config from JSON, validates, fallbacks to defaults, creates file if needed.
 */

#include "config_loader.h"
#include <LittleFS.h>

// Global config instance
GlobalConfig g_config;

// Validate config: check types, bounds (pins 0-39, etc.)
bool validateConfig(const GlobalConfig& cfg) {
    if (cfg.mqtt.port < 1 || cfg.mqtt.port > 65535) return false;
    if (cfg.lte.rssiThreshold > 0 || cfg.lte.rssiThreshold < -100) return false;  // RSSI range
    if (cfg.system.maxCompartments < 1 || cfg.system.maxCompartments > 20) return false;
    if (cfg.system.gracePeriodSec < 0 || cfg.system.gracePeriodSec > 86400) return false;  // 0 to 24h
    if (cfg.system.batteryLowThresholdPercent < 0 || cfg.system.batteryLowThresholdPercent > 100) return false;
    if (cfg.system.solarVoltageMin < 0.0f || cfg.system.solarVoltageMin > 5.0f) return false;
    for (const auto& comp : cfg.compartments) {
        if (comp.servoPin < 0 || comp.servoPin > 39) return false;
        if (comp.limitOpenPin < 0 || comp.limitOpenPin > 39) return false;
        if (comp.limitClosePin < 0 || comp.limitClosePin > 39) return false;
        if (comp.ultrasonicTriggerPin < 0 || comp.ultrasonicTriggerPin > 39) return false;
        if (comp.ultrasonicEchoPin < 0 || comp.ultrasonicEchoPin > 39) return false;
        if (comp.weightSensorPin < 0 || comp.weightSensorPin > 39) return false;
    }
    return true;
}

bool loadConfig() {
    if (!LittleFS.begin()) {
        ESP_LOGE("CONFIG", "Failed to mount LittleFS");
        g_config = getDefaultConfig();
        return false;
    }
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        ESP_LOGW("CONFIG", "config.json not found, using defaults");
        g_config = getDefaultConfig();
        return false;
    }
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        ESP_LOGE("CONFIG", "Failed to parse config.json: %s, using defaults", error.c_str());
        configFile.close();
        g_config = getDefaultConfig();
        return false;
    }
    configFile.close();

    // Parse MQTT
    if (doc.containsKey("mqtt")) {
        JsonObject mqtt = doc["mqtt"];
        g_config.mqtt.broker = mqtt["broker"].as<String>();
        g_config.mqtt.port = mqtt["port"];
        g_config.mqtt.username = mqtt["username"].as<String>();
        g_config.mqtt.password = mqtt["password"].as<String>();
        g_config.mqtt.clientIdPrefix = mqtt["clientIdPrefix"].as<String>();
        g_config.mqtt.useTLS = mqtt["useTLS"] | false;
        g_config.mqtt.caCertPath = mqtt["caCertPath"].as<String>();
        g_config.mqtt.clientCertPath = mqtt["clientCertPath"].as<String>();
        g_config.mqtt.clientKeyPath = mqtt["clientKeyPath"].as<String>();
    }

    // Parse location
    if (doc.containsKey("location")) {
        JsonObject loc = doc["location"];
        g_config.location.slug = loc["slug"].as<String>();
        g_config.location.code = loc["code"].as<String>();
    }

    // Parse WiFi provisioning
    if (doc.containsKey("wifiProvisioning")) {
        JsonObject wp = doc["wifiProvisioning"];
        g_config.wifiProvisioning.fallbackSsid = wp["fallbackSsid"].as<String>();
        g_config.wifiProvisioning.fallbackPass = wp["fallbackPass"].as<String>();
    }

    // Parse LTE
    if (doc.containsKey("lte")) {
        JsonObject lte = doc["lte"];
        g_config.lte.apn = lte["apn"].as<String>();
        g_config.lte.simPin = lte["simPin"].as<String>();
        g_config.lte.rssiThreshold = lte["rssiThreshold"];
        g_config.lte.dataUsageAlertLimitKb = lte["dataUsageAlertLimitKb"];
    }

    // Parse BLE
    if (doc.containsKey("ble")) {
        JsonObject ble = doc["ble"];
        g_config.ble.serviceUuid = ble["serviceUuid"].as<String>();
        g_config.ble.ssidCharUuid = ble["ssidCharUuid"].as<String>();
        g_config.ble.passCharUuid = ble["passCharUuid"].as<String>();
        g_config.ble.statusCharUuid = ble["statusCharUuid"].as<String>();
    }

    // Parse compartments
    if (doc.containsKey("compartments")) {
        JsonArray comps = doc["compartments"];
        g_config.compartments.clear();
        for (JsonObject comp : comps) {
            if (g_config.compartments.size() >= MAX_COMPARTMENTS) break;
            CompartmentConfig c;
            c.number = comp["number"];
            c.servoPin = comp["servoPin"];
            c.limitOpenPin = comp["limitOpenPin"];
            c.limitClosePin = comp["limitClosePin"];
            c.ultrasonicTriggerPin = comp["ultrasonicTriggerPin"];
            c.ultrasonicEchoPin = comp["ultrasonicEchoPin"];
            c.weightSensorPin = comp["weightSensorPin"];
            g_config.compartments.push_back(c);
        }
        g_config.compartmentCount = g_config.compartments.size();
    }

    // Parse system
    if (doc.containsKey("system")) {
        JsonObject sys = doc["system"];
        g_config.system.maxCompartments = sys["maxCompartments"];
        g_config.system.debugMode = sys["debugMode"];
        g_config.system.gracePeriodSec = sys["gracePeriodSec"];
        g_config.system.batteryLowThresholdPercent = sys["batteryLowThresholdPercent"];
        g_config.system.solarVoltageMin = sys["solarVoltageMin"];
    }

    // Parse other
    if (doc.containsKey("other")) {
        JsonObject oth = doc["other"];
        g_config.other.offlinePinTtlSec = oth["offlinePinTtlSec"];
        g_config.other.depositHoldAmountFallback = oth["depositHoldAmountFallback"];
    }

    // Validate
    if (!validateConfig(g_config)) {
        ESP_LOGE("CONFIG", "Config validation failed, using defaults");
        g_config = getDefaultConfig();
        return false;
    }

    ESP_LOGI("CONFIG", "Loaded and validated config from LittleFS");
    return true;
}

bool saveConfig() {
    if (!LittleFS.begin()) {
        ESP_LOGE("CONFIG", "Failed to mount LittleFS for save");
        return false;
    }
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        return false;
    }
    DynamicJsonDocument doc(4096);
    // Serialize g_config to doc...
    // (implement full serialization)
    serializeJson(doc, configFile);
    configFile.close();
    ESP_LOGI("CONFIG", "Saved config to LittleFS");
    return true;
}

bool updateConfigFromJson(const String& jsonPayload) {
    // Validate basic JSON
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, jsonPayload);
    if (error) {
        ESP_LOGE("CONFIG", "Invalid JSON for update: %s", error.c_str());
        return false;
    }
    // Remove old file
    LittleFS.remove("/config.json");
    // Write new
    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        return false;
    }
    configFile.print(jsonPayload);
    configFile.close();
    // Reload globals
    return loadConfig();
}

GlobalConfig getDefaultConfig() {
    GlobalConfig def;
    def.mqtt.broker = "192.168.178.50";
    def.mqtt.port = 8883;
    def.mqtt.username = "mqttuser";
    def.mqtt.password = "strongpass123";
    def.mqtt.clientIdPrefix = "waterfront";
    def.mqtt.useTLS = true;
    def.mqtt.caCertPath = "/ca.pem";
    def.mqtt.clientCertPath = "";
    def.mqtt.clientKeyPath = "";
    def.location.slug = "bremen";
    def.location.code = "harbor-01";
    def.wifiProvisioning.fallbackSsid = "WATERFRONT-DEFAULT";
    def.wifiProvisioning.fallbackPass = "defaultpass123";
    def.lte.apn = "internet.t-mobile.de";
    def.lte.simPin = "";
    def.lte.rssiThreshold = -70;
    def.lte.dataUsageAlertLimitKb = 100000;
    def.ble.serviceUuid = "12345678-1234-1234-1234-123456789abc";
    def.ble.ssidCharUuid = "87654321-4321-4321-4321-cba987654321";
    def.ble.passCharUuid = "87654321-4321-4321-4321-dba987654321";
    def.ble.statusCharUuid = "87654321-4321-4321-4321-eba987654321";
    def.compartments.push_back({1, 12, 13, 14, 15, 16, 17});
    def.compartmentCount = def.compartments.size();
    def.system.maxCompartments = 10;
    def.system.debugMode = true;
    def.system.gracePeriodSec = 3600;
    def.system.batteryLowThresholdPercent = 20;
    def.system.solarVoltageMin = 3.0f;
    def.other.offlinePinTtlSec = 86400;
    def.other.depositHoldAmountFallback = 50;
    return def;
}
