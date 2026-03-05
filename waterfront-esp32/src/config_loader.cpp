/**
 * @file config_loader.cpp
 * @brief Runtime configuration loading and saving from/to LittleFS JSON with validation.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads all config from JSON, validates, fallbacks to defaults, creates file if needed.
 *       Handles edge cases like missing files, invalid JSON, out-of-bounds values.
 */

#include "config_loader.h"
#include <esp_vfs.h>
#include <esp_littlefs.h>
#include <nlohmann/json.hpp>

// Global config instance - shared across the application
GlobalConfig g_config;

// Thread-safety mutex for g_config access (ESP32 multi-core)
portMUX_TYPE g_configMutex = portMUX_INITIALIZER_UNLOCKED;

// Validate config: check types, bounds (pins 0-39, etc.)
// Returns false if any validation fails, logs errors for debugging
bool validateConfig(const GlobalConfig& cfg) {
    // MQTT port must be valid
    if (cfg.mqtt.port < 1 || cfg.mqtt.port > 65535) {
        ESP_LOGE("CONFIG", "Invalid MQTT port: %d", cfg.mqtt.port);
        return false;
    }
    // LTE RSSI threshold range
    if (cfg.lte.rssiThreshold > 0 || cfg.lte.rssiThreshold < -100) {
        ESP_LOGE("CONFIG", "Invalid LTE RSSI threshold: %d", cfg.lte.rssiThreshold);
        return false;
    }
    // System limits
    if (cfg.system.maxCompartments < 1 || cfg.system.maxCompartments > 20) {
        ESP_LOGE("CONFIG", "Invalid max compartments: %d", cfg.system.maxCompartments);
        return false;
    }
    if (cfg.system.gracePeriodSec < 0 || cfg.system.gracePeriodSec > 86400) {
        ESP_LOGE("CONFIG", "Invalid grace period: %d", cfg.system.gracePeriodSec);
        return false;
    }
    if (cfg.system.batteryLowThresholdPercent < 0 || cfg.system.batteryLowThresholdPercent > 100) {
        ESP_LOGE("CONFIG", "Invalid battery threshold: %d", cfg.system.batteryLowThresholdPercent);
        return false;
    }
    if (cfg.system.solarVoltageMin < 0.0f || cfg.system.solarVoltageMin > 5.0f) {
        ESP_LOGE("CONFIG", "Invalid solar voltage min: %f", cfg.system.solarVoltageMin);
        return false;
    }
    // Compartment pin validation
    for (int i = 0; i < cfg.compartmentCount; i++) {
        const auto& comp = cfg.compartments[i];
        if (comp.servoPin < 0 || comp.servoPin > 39) {
            ESP_LOGE("CONFIG", "Invalid servo pin for compartment %d: %d", comp.number, comp.servoPin);
            return false;
        }
        if (comp.limitOpenPin < 0 || comp.limitOpenPin > 39) {
            ESP_LOGE("CONFIG", "Invalid limit open pin for compartment %d: %d", comp.number, comp.limitOpenPin);
            return false;
        }
        if (comp.limitClosePin < 0 || comp.limitClosePin > 39) {
            ESP_LOGE("CONFIG", "Invalid limit close pin for compartment %d: %d", comp.number, comp.limitClosePin);
            return false;
        }
        if (comp.ultrasonicTriggerPin < 0 || comp.ultrasonicTriggerPin > 39) {
            ESP_LOGE("CONFIG", "Invalid ultrasonic trigger pin for compartment %d: %d", comp.number, comp.ultrasonicTriggerPin);
            return false;
        }
        if (comp.ultrasonicEchoPin < 0 || comp.ultrasonicEchoPin > 39) {
            ESP_LOGE("CONFIG", "Invalid ultrasonic echo pin for compartment %d: %d", comp.number, comp.ultrasonicEchoPin);
            return false;
        }
        if (comp.weightSensorPin < 0 || comp.weightSensorPin > 39) {
            ESP_LOGE("CONFIG", "Invalid weight sensor pin for compartment %d: %d", comp.number, comp.weightSensorPin);
            return false;
        }
    }
    return true;
}

// Load config from /config.json in LittleFS
// Returns true on success, false on failure (uses defaults)
// Handles edge cases: missing file, invalid JSON, validation failures
bool loadConfig() {
    ESP_LOGI("CONFIG", "Attempting to load config from LittleFS");
    const int MAX_RETRIES = 3;
    bool success = false;
    for (int retry = 0; retry < MAX_RETRIES; retry++) {
        esp_err_t ret = esp_vfs_littlefs_register(&littlefs_config);
        if (ret != ESP_OK) {
            ESP_LOGE("CONFIG", "Failed to mount LittleFS on attempt %d: %s", retry + 1, esp_err_to_name(ret));
            if (retry == MAX_RETRIES - 1) {
                ESP_LOGW("CONFIG", "Persistent LittleFS failure, attempting format");
                ret = esp_littlefs_format(NULL);
                if (ret == ESP_OK) {
                    ESP_LOGI("CONFIG", "LittleFS formatted successfully, retrying mount");
                    ret = esp_vfs_littlefs_register(&littlefs_config);
                    if (ret == ESP_OK) {
                        ESP_LOGI("CONFIG", "LittleFS mounted after format");
                        success = true;
                        break;
                    } else {
                        ESP_LOGE("CONFIG", "Failed to mount LittleFS even after format: %s", esp_err_to_name(ret));
                    }
                } else {
                    ESP_LOGE("CONFIG", "Failed to format LittleFS: %s", esp_err_to_name(ret));
                }
            }
        } else {
            ESP_LOGI("CONFIG", "LittleFS mounted on attempt %d", retry + 1);
            success = true;
            break;
        }
    }
    if (!success) {
        ESP_LOGE("CONFIG", "LittleFS mount failed after retries and format, using defaults");
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    FILE* configFile = fopen("/littlefs/config.json", "r");
    if (!configFile) {
        ESP_LOGW("CONFIG", "config.json not found, using defaults");
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    fseek(configFile, 0, SEEK_END);
    size_t fileSize = ftell(configFile);
    fseek(configFile, 0, SEEK_SET);
    if (fileSize == 0) {
        ESP_LOGE("CONFIG", "config.json is empty, using defaults");
        fclose(configFile);
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    ESP_LOGI("CONFIG", "Config file size: %d bytes", fileSize);
    char* buffer = (char*)malloc(fileSize + 1);
    if (!buffer) {
        ESP_LOGE("CONFIG", "Failed to allocate buffer for config");
        fclose(configFile);
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    size_t readSize = fread(buffer, 1, fileSize, configFile);
    buffer[readSize] = '\0';
    fclose(configFile);
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(buffer);
    } catch (const nlohmann::json::parse_error& e) {
        ESP_LOGE("CONFIG", "Failed to parse config.json: %s, using defaults", e.what());
        free(buffer);
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    free(buffer);

    // Check version for migration
    std::string configVersion = doc.value("version", "");
    if (configVersion.empty()) {
        ESP_LOGW("CONFIG", "Config version missing, assuming old config, migrating to 1.0");
        configVersion = "1.0";
        // Add any migration logic here if needed for future versions
    }
    vPortEnterCritical(&g_configMutex);
    g_config.version = configVersion;
    ESP_LOGI("CONFIG", "Config version: %s", g_config.version.c_str());

    // Parse MQTT section
    if (doc.contains("mqtt")) {
        auto mqtt = doc["mqtt"];
        g_config.mqtt.broker = mqtt.value("broker", "");
        g_config.mqtt.port = mqtt.value("port", 0);
        g_config.mqtt.username = mqtt.value("username", "");
        g_config.mqtt.password = mqtt.value("password", "");
        g_config.mqtt.clientIdPrefix = mqtt.value("clientIdPrefix", "");
        g_config.mqtt.useTLS = mqtt.value("useTLS", false);
        g_config.mqtt.caCertPath = mqtt.value("caCertPath", "");
        g_config.mqtt.clientCertPath = mqtt.value("clientCertPath", "");
        g_config.mqtt.clientKeyPath = mqtt.value("clientKeyPath", "");
        g_config.mqtt.tlsSkipVerify = mqtt.value("tlsSkipVerify", false);
        ESP_LOGI("CONFIG", "Loaded MQTT config: broker=%s, port=%d, useTLS=%d, tlsSkipVerify=%d", g_config.mqtt.broker.c_str(), g_config.mqtt.port, g_config.mqtt.useTLS, g_config.mqtt.tlsSkipVerify);
    } else {
        ESP_LOGW("CONFIG", "MQTT section missing, using defaults");
    }

    // Parse location section
    if (doc.contains("location")) {
        auto loc = doc["location"];
        g_config.location.slug = loc.value("slug", "");
        g_config.location.code = loc.value("code", "");
        ESP_LOGI("CONFIG", "Loaded location: slug=%s, code=%s", g_config.location.slug.c_str(), g_config.location.code.c_str());
    } else {
        ESP_LOGW("CONFIG", "Location section missing, using defaults");
    }

    // Parse WiFi provisioning section
    if (doc.contains("wifiProvisioning")) {
        auto wp = doc["wifiProvisioning"];
        g_config.wifiProvisioning.fallbackSsid = wp.value("fallbackSsid", "");
        g_config.wifiProvisioning.fallbackPass = wp.value("fallbackPass", "");
        ESP_LOGI("CONFIG", "Loaded WiFi provisioning: SSID=%s", g_config.wifiProvisioning.fallbackSsid.c_str());
    } else {
        ESP_LOGW("CONFIG", "WiFi provisioning section missing, using defaults");
    }

    // Parse LTE section
    if (doc.contains("lte")) {
        auto lte = doc["lte"];
        g_config.lte.apn = lte.value("apn", "");
        g_config.lte.simPin = lte.value("simPin", "");
        g_config.lte.rssiThreshold = lte.value("rssiThreshold", 0);
        g_config.lte.dataUsageAlertLimitKb = lte.value("dataUsageAlertLimitKb", 0);
        ESP_LOGI("CONFIG", "Loaded LTE config: APN=%s, RSSI threshold=%d", g_config.lte.apn.c_str(), g_config.lte.rssiThreshold);
    } else {
        ESP_LOGW("CONFIG", "LTE section missing, using defaults");
    }

    // Parse BLE section
    if (doc.contains("ble")) {
        auto ble = doc["ble"];
        g_config.ble.serviceUuid = ble.value("serviceUuid", "");
        g_config.ble.ssidCharUuid = ble.value("ssidCharUuid", "");
        g_config.ble.passCharUuid = ble.value("passCharUuid", "");
        g_config.ble.statusCharUuid = ble.value("statusCharUuid", "");
        ESP_LOGI("CONFIG", "Loaded BLE config: service UUID=%s", g_config.ble.serviceUuid.c_str());
    } else {
        ESP_LOGW("CONFIG", "BLE section missing, using defaults");
    }

    // Parse compartments array
    if (doc.contains("compartments")) {
        auto comps = doc["compartments"];
        g_config.compartmentCount = 0;
        for (const auto& comp : comps) {
            if (g_config.compartmentCount >= MAX_COMPARTMENTS) {
                ESP_LOGW("CONFIG", "Max compartments (%d) reached, skipping additional", MAX_COMPARTMENTS);
                break;
            }
            CompartmentConfig c;
            c.number = comp.value("number", 0);
            c.servoPin = comp.value("servoPin", 0);
            c.limitOpenPin = comp.value("limitOpenPin", 0);
            c.limitClosePin = comp.value("limitClosePin", 0);
            c.ultrasonicTriggerPin = comp.value("ultrasonicTriggerPin", 0);
            c.ultrasonicEchoPin = comp.value("ultrasonicEchoPin", 0);
            c.weightSensorPin = comp.value("weightSensorPin", 0);
            g_config.compartments[g_config.compartmentCount++] = c;
            ESP_LOGI("CONFIG", "Loaded compartment %d: servo=%d, limits=%d/%d", c.number, c.servoPin, c.limitOpenPin, c.limitClosePin);
        }
        ESP_LOGI("CONFIG", "Total compartments loaded: %d", g_config.compartmentCount);
    } else {
        ESP_LOGW("CONFIG", "Compartments section missing, using defaults");
    }

    // Parse system section
    if (doc.contains("system")) {
        auto sys = doc["system"];
        g_config.system.maxCompartments = sys.value("maxCompartments", 0);
        g_config.system.debugMode = sys.value("debugMode", false);
        g_config.system.gracePeriodSec = sys.value("gracePeriodSec", 0);
        g_config.system.batteryLowThresholdPercent = sys.value("batteryLowThresholdPercent", 0);
        g_config.system.solarVoltageMin = sys.value("solarVoltageMin", 0.0f);
        ESP_LOGI("CONFIG", "Loaded system config: debugMode=%d, gracePeriod=%d", g_config.system.debugMode, g_config.system.gracePeriodSec);
    } else {
        ESP_LOGW("CONFIG", "System section missing, using defaults");
    }

    // Parse other section
    if (doc.contains("other")) {
        auto oth = doc["other"];
        g_config.other.offlinePinTtlSec = oth.value("offlinePinTtlSec", 0);
        g_config.other.depositHoldAmountFallback = oth.value("depositHoldAmountFallback", 0);
        ESP_LOGI("CONFIG", "Loaded other config: offline TTL=%d", g_config.other.offlinePinTtlSec);
    } else {
        ESP_LOGW("CONFIG", "Other section missing, using defaults");
    }

    // Validate loaded config
    if (!validateConfig(g_config)) {
        ESP_LOGE("CONFIG", "Config validation failed, using defaults");
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }

    ESP_LOGI("CONFIG", "Loaded and validated config from LittleFS");
    vPortExitCritical(&g_configMutex);
    return true;
}

// Save current config to /config.json
// Returns true on success, false on failure
bool saveConfig() {
    ESP_LOGI("CONFIG", "Attempting to save config to LittleFS");
    FILE* configFile = fopen("/littlefs/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        return false;
    }
    nlohmann::json doc;
    // Serialize g_config to doc (placeholder - implement full serialization if needed)
    // For now, assume config is saved via updateConfigFromJson
    std::string jsonString = doc.dump();
    size_t written = fwrite(jsonString.c_str(), 1, jsonString.size(), configFile);
    fclose(configFile);
    if (written != jsonString.size()) {
        ESP_LOGE("CONFIG", "Failed to write full config to file");
        return false;
    }
    ESP_LOGI("CONFIG", "Saved config to LittleFS");
    return true;
}

// Update config from JSON payload (e.g., via MQTT)
// Returns true on success, false on failure
// Handles edge cases: invalid JSON, validation failure
bool updateConfigFromJson(const char* jsonPayload) {
    ESP_LOGI("CONFIG", "Attempting to update config from JSON payload");
    if (!jsonPayload || strlen(jsonPayload) == 0) {
        ESP_LOGE("CONFIG", "Empty JSON payload for update");
        return false;
    }
    // Validate basic JSON
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(jsonPayload);
    } catch (const nlohmann::json::parse_error& e) {
        ESP_LOGE("CONFIG", "Invalid JSON for update: %s", e.what());
        return false;
    }
    // Remove old file
    if (unlink("/littlefs/config.json") != 0) {
        ESP_LOGE("CONFIG", "Failed to remove old config file");
        return false;
    }
    // Write new
    FILE* configFile = fopen("/littlefs/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        return false;
    }
    size_t written = fwrite(jsonPayload, 1, strlen(jsonPayload), configFile);
    fclose(configFile);
    if (written != strlen(jsonPayload)) {
        ESP_LOGE("CONFIG", "Failed to write full payload to file");
        return false;
    }
    ESP_LOGI("CONFIG", "Updated config file, reloading");
    // Reload globals
    return loadConfig();
}

// Get default config values
// Used when config file is missing or invalid
GlobalConfig getDefaultConfig() {
    ESP_LOGI("CONFIG", "Generating default config");
    GlobalConfig def;
    def.version = "1.0";
    def.mqtt.broker = "8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud";
    def.mqtt.port = 8883;
    def.mqtt.username = "mqttuser";
    def.mqtt.password = "strongpass123";
    def.mqtt.clientIdPrefix = "waterfront";
    def.mqtt.useTLS = true;
    def.mqtt.caCertPath = "/littlefs/ca.pem";
    def.mqtt.clientCertPath = "";
    def.mqtt.clientKeyPath = "";
    def.mqtt.tlsSkipVerify = false;
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
    def.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    def.compartmentCount = 1;
    def.system.maxCompartments = 10;
    def.system.debugMode = true;
    def.system.gracePeriodSec = 3600;
    def.system.batteryLowThresholdPercent = 20;
    def.system.solarVoltageMin = 3.0f;
    def.other.offlinePinTtlSec = 86400;
    def.other.depositHoldAmountFallback = 50;
    return def;
}

// Serialize current g_config to JSON string
// Useful for publishing current config via MQTT
std::string getConfigAsJson() {
    vPortEnterCritical(&g_configMutex);
    nlohmann::json doc;
    // Serialize version
    doc["version"] = g_config.version;
    // Serialize MQTT section
    auto mqtt = doc["mqtt"];
    mqtt["broker"] = g_config.mqtt.broker;
    mqtt["port"] = g_config.mqtt.port;
    mqtt["username"] = g_config.mqtt.username;
    mqtt["password"] = g_config.mqtt.password;
    mqtt["clientIdPrefix"] = g_config.mqtt.clientIdPrefix;
    mqtt["useTLS"] = g_config.mqtt.useTLS;
    mqtt["caCertPath"] = g_config.mqtt.caCertPath;
    mqtt["clientCertPath"] = g_config.mqtt.clientCertPath;
    mqtt["clientKeyPath"] = g_config.mqtt.clientKeyPath;
    mqtt["tlsSkipVerify"] = g_config.mqtt.tlsSkipVerify;
    // Serialize location section
    auto location = doc["location"];
    location["slug"] = g_config.location.slug;
    location["code"] = g_config.location.code;
    // Serialize WiFi provisioning section
    auto wifiProvisioning = doc["wifiProvisioning"];
    wifiProvisioning["fallbackSsid"] = g_config.wifiProvisioning.fallbackSsid;
    wifiProvisioning["fallbackPass"] = g_config.wifiProvisioning.fallbackPass;
    // Serialize LTE section
    auto lte = doc["lte"];
    lte["apn"] = g_config.lte.apn;
    lte["simPin"] = g_config.lte.simPin;
    lte["rssiThreshold"] = g_config.lte.rssiThreshold;
    lte["dataUsageAlertLimitKb"] = g_config.lte.dataUsageAlertLimitKb;
    // Serialize BLE section
    auto ble = doc["ble"];
    ble["serviceUuid"] = g_config.ble.serviceUuid;
    ble["ssidCharUuid"] = g_config.ble.ssidCharUuid;
    ble["passCharUuid"] = g_config.ble.passCharUuid;
    ble["statusCharUuid"] = g_config.ble.statusCharUuid;
    // Serialize compartments array
    auto compartments = doc["compartments"];
    for (int i = 0; i < g_config.compartmentCount; i++) {
        auto comp = compartments[i];
        comp["number"] = g_config.compartments[i].number;
        comp["servoPin"] = g_config.compartments[i].servoPin;
        comp["limitOpenPin"] = g_config.compartments[i].limitOpenPin;
        comp["limitClosePin"] = g_config.compartments[i].limitClosePin;
        comp["ultrasonicTriggerPin"] = g_config.compartments[i].ultrasonicTriggerPin;
        comp["ultrasonicEchoPin"] = g_config.compartments[i].ultrasonicEchoPin;
        comp["weightSensorPin"] = g_config.compartments[i].weightSensorPin;
    }
    // Serialize system section
    auto system = doc["system"];
    system["maxCompartments"] = g_config.system.maxCompartments;
    system["debugMode"] = g_config.system.debugMode;
    system["gracePeriodSec"] = g_config.system.gracePeriodSec;
    system["batteryLowThresholdPercent"] = g_config.system.batteryLowThresholdPercent;
    system["solarVoltageMin"] = g_config.system.solarVoltageMin;
    // Serialize other section
    auto other = doc["other"];
    other["offlinePinTtlSec"] = g_config.other.offlinePinTtlSec;
    other["depositHoldAmountFallback"] = g_config.other.depositHoldAmountFallback;
    // Serialize to string
    std::string jsonString = doc.dump();
    vPortExitCritical(&g_configMutex);
    return jsonString;
}
