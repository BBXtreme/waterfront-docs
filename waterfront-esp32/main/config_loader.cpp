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
#include <cJSON.h>

// LittleFS configuration
static esp_littlefs_config_t littlefs_config = {
    .base_path = "/littlefs",
    .partition_label = NULL,
    .format_if_mount_failed = true,
    .dont_mount = false
};

// Global config instance - shared across the application
GlobalConfig g_config;

// Thread-safety mutex for g_config access (ESP32 multi-core)
portMUX_TYPE g_configMutex = portMUX_INITIALIZER_UNLOCKED;

// Static buffer for JSON operations to avoid dynamic allocation
static char jsonBuffer[JSON_BUFFER_SIZE];

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
    // Log level validation
    if (cfg.system.logLevel < 0 || cfg.system.logLevel > 5) {
        ESP_LOGE("CONFIG", "Invalid log level: %d", cfg.system.logLevel);
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
    if (fileSize == 0 || fileSize >= sizeof(jsonBuffer)) {
        ESP_LOGE("CONFIG", "config.json is empty or too large, using defaults");
        fclose(configFile);
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }
    ESP_LOGI("CONFIG", "Config file size: %d bytes", fileSize);
    size_t readSize = fread(jsonBuffer, 1, fileSize, configFile);
    jsonBuffer[readSize] = '\0';
    fclose(configFile);
    cJSON *doc = cJSON_Parse(jsonBuffer);
    if (!doc) {
        ESP_LOGE("CONFIG", "Failed to parse config.json, using defaults");
        vPortEnterCritical(&g_configMutex);
        g_config = getDefaultConfig();
        vPortExitCritical(&g_configMutex);
        return false;
    }

    // Check version for migration
    cJSON *versionItem = cJSON_GetObjectItem(doc, "version");
    const char *configVersion = cJSON_GetStringValue(versionItem);
    if (!configVersion) {
        ESP_LOGW("CONFIG", "Config version missing, assuming old config, migrating to 1.0");
        configVersion = "1.0";
        // Add any migration logic here if needed for future versions
    }
    vPortEnterCritical(&g_configMutex);
    strcpy(g_config.version, configVersion);
    ESP_LOGI("CONFIG", "Config version: %s", g_config.version);

    // Parse MQTT section
    if (cJSON_HasObjectItem(doc, "mqtt")) {
        cJSON *mqtt = cJSON_GetObjectItem(doc, "mqtt");
        const char *broker = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "broker"));
        strcpy(g_config.mqtt.broker, broker ? broker : "");
        g_config.mqtt.port = cJSON_GetNumberValue(cJSON_GetObjectItem(mqtt, "port"));
        const char *username = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "username"));
        strcpy(g_config.mqtt.username, username ? username : "");
        const char *password = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "password"));
        strcpy(g_config.mqtt.password, password ? password : "");
        const char *clientIdPrefix = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "clientIdPrefix"));
        strcpy(g_config.mqtt.clientIdPrefix, clientIdPrefix ? clientIdPrefix : "");
        g_config.mqtt.useTLS = cJSON_IsTrue(cJSON_GetObjectItem(mqtt, "useTLS"));
        const char *caCertPath = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "caCertPath"));
        strcpy(g_config.mqtt.caCertPath, caCertPath ? caCertPath : "");
        const char *clientCertPath = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "clientCertPath"));
        strcpy(g_config.mqtt.clientCertPath, clientCertPath ? clientCertPath : "");
        const char *clientKeyPath = cJSON_GetStringValue(cJSON_GetObjectItem(mqtt, "clientKeyPath"));
        strcpy(g_config.mqtt.clientKeyPath, clientKeyPath ? clientKeyPath : "");
        ESP_LOGI("CONFIG", "Loaded MQTT config: broker=%s, port=%d, useTLS=%d", g_config.mqtt.broker, g_config.mqtt.port, g_config.mqtt.useTLS);
    } else {
        ESP_LOGW("CONFIG", "MQTT section missing, using defaults");
    }

    // Parse location section
    if (cJSON_HasObjectItem(doc, "location")) {
        cJSON *loc = cJSON_GetObjectItem(doc, "location");
        const char *slug = cJSON_GetStringValue(cJSON_GetObjectItem(loc, "slug"));
        strcpy(g_config.location.slug, slug ? slug : "");
        const char *code = cJSON_GetStringValue(cJSON_GetObjectItem(loc, "code"));
        strcpy(g_config.location.code, code ? code : "");
        ESP_LOGI("CONFIG", "Loaded location: slug=%s, code=%s", g_config.location.slug, g_config.location.code);
    } else {
        ESP_LOGW("CONFIG", "Location section missing, using defaults");
    }

    // Parse WiFi provisioning section
    if (cJSON_HasObjectItem(doc, "wifiProvisioning")) {
        cJSON *wp = cJSON_GetObjectItem(doc, "wifiProvisioning");
        const char *fallbackSsid = cJSON_GetStringValue(cJSON_GetObjectItem(wp, "fallbackSsid"));
        strcpy(g_config.wifiProvisioning.fallbackSsid, fallbackSsid ? fallbackSsid : "");
        const char *fallbackPass = cJSON_GetStringValue(cJSON_GetObjectItem(wp, "fallbackPass"));
        strcpy(g_config.wifiProvisioning.fallbackPass, fallbackPass ? fallbackPass : "");
        ESP_LOGI("CONFIG", "Loaded WiFi provisioning: SSID=%s", g_config.wifiProvisioning.fallbackSsid);
    } else {
        ESP_LOGW("CONFIG", "WiFi provisioning section missing, using defaults");
    }

    // Parse LTE section
    if (cJSON_HasObjectItem(doc, "lte")) {
        cJSON *lte = cJSON_GetObjectItem(doc, "lte");
        const char *apn = cJSON_GetStringValue(cJSON_GetObjectItem(lte, "apn"));
        strcpy(g_config.lte.apn, apn ? apn : "");
        const char *simPin = cJSON_GetStringValue(cJSON_GetObjectItem(lte, "simPin"));
        strcpy(g_config.lte.simPin, simPin ? simPin : "");
        g_config.lte.rssiThreshold = cJSON_GetNumberValue(cJSON_GetObjectItem(lte, "rssiThreshold"));
        g_config.lte.dataUsageAlertLimitKb = cJSON_GetNumberValue(cJSON_GetObjectItem(lte, "dataUsageAlertLimitKb"));
        ESP_LOGI("CONFIG", "Loaded LTE config: APN=%s, RSSI threshold=%d", g_config.lte.apn, g_config.lte.rssiThreshold);
    } else {
        ESP_LOGW("CONFIG", "LTE section missing, using defaults");
    }

    // Parse BLE section
    if (cJSON_HasObjectItem(doc, "ble")) {
        cJSON *ble = cJSON_GetObjectItem(doc, "ble");
        const char *serviceUuid = cJSON_GetStringValue(cJSON_GetObjectItem(ble, "serviceUuid"));
        strcpy(g_config.ble.serviceUuid, serviceUuid ? serviceUuid : "");
        const char *ssidCharUuid = cJSON_GetStringValue(cJSON_GetObjectItem(ble, "ssidCharUuid"));
        strcpy(g_config.ble.ssidCharUuid, ssidCharUuid ? ssidCharUuid : "");
        const char *passCharUuid = cJSON_GetStringValue(cJSON_GetObjectItem(ble, "passCharUuid"));
        strcpy(g_config.ble.passCharUuid, passCharUuid ? passCharUuid : "");
        const char *statusCharUuid = cJSON_GetStringValue(cJSON_GetObjectItem(ble, "statusCharUuid"));
        strcpy(g_config.ble.statusCharUuid, statusCharUuid ? statusCharUuid : "");
        ESP_LOGI("CONFIG", "Loaded BLE config: service UUID=%s", g_config.ble.serviceUuid);
    } else {
        ESP_LOGW("CONFIG", "BLE section missing, using defaults");
    }

    // Parse compartments array
    if (cJSON_HasObjectItem(doc, "compartments")) {
        cJSON *comps = cJSON_GetObjectItem(doc, "compartments");
        g_config.compartmentCount = 0;
        int count = cJSON_GetArraySize(comps);
        for (int i = 0; i < count && g_config.compartmentCount < MAX_COMPARTMENTS; i++) {
            cJSON *comp = cJSON_GetArrayItem(comps, i);
            CompartmentConfig c;
            c.number = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "number"));
            c.servoPin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "servoPin"));
            c.limitOpenPin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "limitOpenPin"));
            c.limitClosePin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "limitClosePin"));
            c.ultrasonicTriggerPin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "ultrasonicTriggerPin"));
            c.ultrasonicEchoPin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "ultrasonicEchoPin"));
            c.weightSensorPin = cJSON_GetNumberValue(cJSON_GetObjectItem(comp, "weightSensorPin"));
            g_config.compartments[g_config.compartmentCount++] = c;
            ESP_LOGI("CONFIG", "Loaded compartment %d: servo=%d, limits=%d/%d", c.number, c.servoPin, c.limitOpenPin, c.limitClosePin);
        }
        ESP_LOGI("CONFIG", "Total compartments loaded: %d", g_config.compartmentCount);
    } else {
        ESP_LOGW("CONFIG", "Compartments section missing, using defaults");
    }

    // Parse system section
    if (cJSON_HasObjectItem(doc, "system")) {
        cJSON *sys = cJSON_GetObjectItem(doc, "system");
        g_config.system.maxCompartments = cJSON_GetNumberValue(cJSON_GetObjectItem(sys, "maxCompartments"));
        g_config.system.debugMode = cJSON_IsTrue(cJSON_GetObjectItem(sys, "debugMode"));
        g_config.system.gracePeriodSec = cJSON_GetNumberValue(cJSON_GetObjectItem(sys, "gracePeriodSec"));
        g_config.system.batteryLowThresholdPercent = cJSON_GetNumberValue(cJSON_GetObjectItem(sys, "batteryLowThresholdPercent"));
        g_config.system.solarVoltageMin = cJSON_GetNumberValue(cJSON_GetObjectItem(sys, "solarVoltageMin"));
        g_config.system.logLevel = cJSON_GetNumberValue(cJSON_GetObjectItem(sys, "logLevel"));
        const char *otaPassword = cJSON_GetStringValue(cJSON_GetObjectItem(sys, "otaPassword"));
        strcpy(g_config.system.otaPassword, otaPassword ? otaPassword : "");
        ESP_LOGI("CONFIG", "Loaded system config: debugMode=%d, gracePeriod=%d, logLevel=%d", g_config.system.debugMode, g_config.system.gracePeriodSec, g_config.system.logLevel);
    } else {
        ESP_LOGW("CONFIG", "System section missing, using defaults");
    }

    // Parse other section
    if (cJSON_HasObjectItem(doc, "other")) {
        cJSON *oth = cJSON_GetObjectItem(doc, "other");
        g_config.other.offlinePinTtlSec = cJSON_GetNumberValue(cJSON_GetObjectItem(oth, "offlinePinTtlSec"));
        g_config.other.depositHoldAmountFallback = cJSON_GetNumberValue(cJSON_GetObjectItem(oth, "depositHoldAmountFallback"));
        ESP_LOGI("CONFIG", "Loaded other config: offline TTL=%d", g_config.other.offlinePinTtlSec);
    } else {
        ESP_LOGW("CONFIG", "Other section missing, using defaults");
    }

    // Validate loaded config
    if (!validateConfig(g_config)) {
        ESP_LOGE("CONFIG", "Config validation failed, using defaults");
        g_config = getDefaultConfig();
        cJSON_Delete(doc);
        vPortExitCritical(&g_configMutex);
        return false;
    }

    ESP_LOGI("CONFIG", "Loaded and validated config from LittleFS");
    cJSON_Delete(doc);
    vPortExitCritical(&g_configMutex);
    return true;
}

// Save current config to /config.json
// Returns true on success, false on failure
bool saveConfig() {
    ESP_LOGI("CONFIG", "Attempting to save config to LittleFS");
    vPortEnterCritical(&g_configMutex);
    cJSON *doc = cJSON_CreateObject();
    // Serialize version
    cJSON_AddStringToObject(doc, "version", g_config.version);
    // Serialize MQTT section
    cJSON *mqtt = cJSON_AddObjectToObject(doc, "mqtt");
    cJSON_AddStringToObject(mqtt, "broker", g_config.mqtt.broker);
    cJSON_AddNumberToObject(mqtt, "port", g_config.mqtt.port);
    cJSON_AddStringToObject(mqtt, "username", g_config.mqtt.username);
    cJSON_AddStringToObject(mqtt, "password", g_config.mqtt.password);
    cJSON_AddStringToObject(mqtt, "clientIdPrefix", g_config.mqtt.clientIdPrefix);
    cJSON_AddBoolToObject(mqtt, "useTLS", g_config.mqtt.useTLS);
    cJSON_AddStringToObject(mqtt, "caCertPath", g_config.mqtt.caCertPath);
    cJSON_AddStringToObject(mqtt, "clientCertPath", g_config.mqtt.clientCertPath);
    cJSON_AddStringToObject(mqtt, "clientKeyPath", g_config.mqtt.clientKeyPath);
    // Serialize location section
    cJSON *loc = cJSON_AddObjectToObject(doc, "location");
    cJSON_AddStringToObject(loc, "slug", g_config.location.slug);
    cJSON_AddStringToObject(loc, "code", g_config.location.code);
    // Serialize WiFi provisioning section
    cJSON *wp = cJSON_AddObjectToObject(doc, "wifiProvisioning");
    cJSON_AddStringToObject(wp, "fallbackSsid", g_config.wifiProvisioning.fallbackSsid);
    cJSON_AddStringToObject(wp, "fallbackPass", g_config.wifiProvisioning.fallbackPass);
    // Serialize LTE section
    cJSON *lte = cJSON_AddObjectToObject(doc, "lte");
    cJSON_AddStringToObject(lte, "apn", g_config.lte.apn);
    cJSON_AddStringToObject(lte, "simPin", g_config.lte.simPin);
    cJSON_AddNumberToObject(lte, "rssiThreshold", g_config.lte.rssiThreshold);
    cJSON_AddNumberToObject(lte, "dataUsageAlertLimitKb", g_config.lte.dataUsageAlertLimitKb);
    // Serialize BLE section
    cJSON *ble = cJSON_AddObjectToObject(doc, "ble");
    cJSON_AddStringToObject(ble, "serviceUuid", g_config.ble.serviceUuid);
    cJSON_AddStringToObject(ble, "ssidCharUuid", g_config.ble.ssidCharUuid);
    cJSON_AddStringToObject(ble, "passCharUuid", g_config.ble.passCharUuid);
    cJSON_AddStringToObject(ble, "statusCharUuid", g_config.ble.statusCharUuid);
    // Serialize compartments array
    cJSON *comps = cJSON_AddArrayToObject(doc, "compartments");
    for (int i = 0; i < g_config.compartmentCount; i++) {
        cJSON *comp = cJSON_CreateObject();
        cJSON_AddNumberToObject(comp, "number", g_config.compartments[i].number);
        cJSON_AddNumberToObject(comp, "servoPin", g_config.compartments[i].servoPin);
        cJSON_AddNumberToObject(comp, "limitOpenPin", g_config.compartments[i].limitOpenPin);
        cJSON_AddNumberToObject(comp, "limitClosePin", g_config.compartments[i].limitClosePin);
        cJSON_AddNumberToObject(comp, "ultrasonicTriggerPin", g_config.compartments[i].ultrasonicTriggerPin);
        cJSON_AddNumberToObject(comp, "ultrasonicEchoPin", g_config.compartments[i].ultrasonicEchoPin);
        cJSON_AddNumberToObject(comp, "weightSensorPin", g_config.compartments[i].weightSensorPin);
        cJSON_AddItemToArray(comps, comp);
    }
    // Serialize system section
    cJSON *sys = cJSON_AddObjectToObject(doc, "system");
    cJSON_AddNumberToObject(sys, "maxCompartments", g_config.system.maxCompartments);
    cJSON_AddBoolToObject(sys, "debugMode", g_config.system.debugMode);
    cJSON_AddNumberToObject(sys, "gracePeriodSec", g_config.system.gracePeriodSec);
    cJSON_AddNumberToObject(sys, "batteryLowThresholdPercent", g_config.system.batteryLowThresholdPercent);
    cJSON_AddNumberToObject(sys, "solarVoltageMin", g_config.system.solarVoltageMin);
    cJSON_AddNumberToObject(sys, "logLevel", g_config.system.logLevel);
    cJSON_AddStringToObject(sys, "otaPassword", g_config.system.otaPassword);
    // Serialize other section
    cJSON *oth = cJSON_AddObjectToObject(doc, "other");
    cJSON_AddNumberToObject(oth, "offlinePinTtlSec", g_config.other.offlinePinTtlSec);
    cJSON_AddNumberToObject(oth, "depositHoldAmountFallback", g_config.other.depositHoldAmountFallback);
    // Serialize to string
    char *jsonString = cJSON_Print(doc);
    vPortExitCritical(&g_configMutex);
    FILE* configFile = fopen("/littlefs/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        cJSON_free(jsonString);
        cJSON_Delete(doc);
        return false;
    }
    size_t written = fwrite(jsonString, 1, strlen(jsonString), configFile);
    fclose(configFile);
    cJSON_free(jsonString);
    cJSON_Delete(doc);
    if (written != strlen(jsonString)) {
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
    if (!jsonPayload || strlen(jsonPayload) == 0 || strlen(jsonPayload) >= sizeof(jsonBuffer)) {
        ESP_LOGE("CONFIG", "Empty or too large JSON payload for update");
        return false;
    }
    // Validate basic JSON
    cJSON *doc = cJSON_Parse(jsonPayload);
    if (!doc) {
        ESP_LOGE("CONFIG", "Invalid JSON for update");
        return false;
    }
    // Remove old file
    if (unlink("/littlefs/config.json") != 0) {
        ESP_LOGE("CONFIG", "Failed to remove old config file");
        cJSON_Delete(doc);
        return false;
    }
    // Write new
    FILE* configFile = fopen("/littlefs/config.json", "w");
    if (!configFile) {
        ESP_LOGE("CONFIG", "Failed to open config.json for write");
        cJSON_Delete(doc);
        return false;
    }
    size_t written = fwrite(jsonPayload, 1, strlen(jsonPayload), configFile);
    fclose(configFile);
    if (written != strlen(jsonPayload)) {
        ESP_LOGE("CONFIG", "Failed to write full payload to file");
        cJSON_Delete(doc);
        return false;
    }
    ESP_LOGI("CONFIG", "Updated config file, reloading");
    cJSON_Delete(doc);
    // Reload globals
    return loadConfig();
}

// Get default config values
// Used when config file is missing or invalid
GlobalConfig getDefaultConfig() {
    ESP_LOGI("CONFIG", "Generating default config");
    GlobalConfig def;
    strcpy(def.version, "1.0");
    strcpy(def.mqtt.broker, "8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud");
    def.mqtt.port = 8883;
    strcpy(def.mqtt.username, "mqttuser");
    strcpy(def.mqtt.password, "strongpass123");
    strcpy(def.mqtt.clientIdPrefix, "waterfront");
    def.mqtt.useTLS = true;
    strcpy(def.mqtt.caCertPath, "/littlefs/ca.pem");
    strcpy(def.mqtt.clientCertPath, "");
    strcpy(def.mqtt.clientKeyPath, "");
    strcpy(def.location.slug, "bremen");
    strcpy(def.location.code, "harbor-01");
    strcpy(def.wifiProvisioning.fallbackSsid, "WATERFRONT-DEFAULT");
    strcpy(def.wifiProvisioning.fallbackPass, "defaultpass123");
    strcpy(def.lte.apn, "internet.t-mobile.de");
    strcpy(def.lte.simPin, "");
    def.lte.rssiThreshold = -70;
    def.lte.dataUsageAlertLimitKb = 100000;
    strcpy(def.ble.serviceUuid, "12345678-1234-1234-1234-123456789abc");
    strcpy(def.ble.ssidCharUuid, "87654321-4321-4321-4321-cba987654321");
    strcpy(def.ble.passCharUuid, "87654321-4321-4321-4321-dba987654321");
    strcpy(def.ble.statusCharUuid, "87654321-4321-4321-4321-eba987654321");
    def.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    def.compartmentCount = 1;
    def.system.maxCompartments = 10;
    def.system.debugMode = true;
    def.system.gracePeriodSec = 3600;
    def.system.batteryLowThresholdPercent = 20;
    def.system.solarVoltageMin = 3.0f;
    def.system.logLevel = LOG_LEVEL_DEFAULT;
    strcpy(def.system.otaPassword, "secureota123");
    def.other.offlinePinTtlSec = 86400;
    def.other.depositHoldAmountFallback = 50;
    return def;
}

// Serialize current g_config to JSON string
// Useful for publishing current config via MQTT
const char* getConfigAsJson() {
    vPortEnterCritical(&g_configMutex);
    cJSON *doc = cJSON_CreateObject();
    // Serialize version
    cJSON_AddStringToObject(doc, "version", g_config.version);
    // Serialize MQTT section
    cJSON *mqtt = cJSON_AddObjectToObject(doc, "mqtt");
    cJSON_AddStringToObject(mqtt, "broker", g_config.mqtt.broker);
    cJSON_AddNumberToObject(mqtt, "port", g_config.mqtt.port);
    cJSON_AddStringToObject(mqtt, "username", g_config.mqtt.username);
    cJSON_AddStringToObject(mqtt, "password", g_config.mqtt.password);
    cJSON_AddStringToObject(mqtt, "clientIdPrefix", g_config.mqtt.clientIdPrefix);
    cJSON_AddBoolToObject(mqtt, "useTLS", g_config.mqtt.useTLS);
    cJSON_AddStringToObject(mqtt, "caCertPath", g_config.mqtt.caCertPath);
    cJSON_AddStringToObject(mqtt, "clientCertPath", g_config.mqtt.clientCertPath);
    cJSON_AddStringToObject(mqtt, "clientKeyPath", g_config.mqtt.clientKeyPath);
    // Serialize location section
    cJSON *loc = cJSON_AddObjectToObject(doc, "location");
    cJSON_AddStringToObject(loc, "slug", g_config.location.slug);
    cJSON_AddStringToObject(loc, "code", g_config.location.code);
    // Serialize WiFi provisioning section
    cJSON *wp = cJSON_AddObjectToObject(doc, "wifiProvisioning");
    cJSON_AddStringToObject(wp, "fallbackSsid", g_config.wifiProvisioning.fallbackSsid);
    cJSON_AddStringToObject(wp, "fallbackPass", g_config.wifiProvisioning.fallbackPass);
    // Serialize LTE section
    cJSON *lte = cJSON_AddObjectToObject(doc, "lte");
    cJSON_AddStringToObject(lte, "apn", g_config.lte.apn);
    cJSON_AddStringToObject(lte, "simPin", g_config.lte.simPin);
    cJSON_AddNumberToObject(lte, "rssiThreshold", g_config.lte.rssiThreshold);
    cJSON_AddNumberToObject(lte, "dataUsageAlertLimitKb", g_config.lte.dataUsageAlertLimitKb);
    // Serialize BLE section
    cJSON *ble = cJSON_AddObjectToObject(doc, "ble");
    cJSON_AddStringToObject(ble, "serviceUuid", g_config.ble.serviceUuid);
    cJSON_AddStringToObject(ble, "ssidCharUuid", g_config.ble.ssidCharUuid);
    cJSON_AddStringToObject(ble, "passCharUuid", g_config.ble.passCharUuid);
    cJSON_AddStringToObject(ble, "statusCharUuid", g_config.ble.statusCharUuid);
    // Serialize compartments array
    cJSON *comps = cJSON_AddArrayToObject(doc, "compartments");
    for (int i = 0; i < g_config.compartmentCount; i++) {
        cJSON *comp = cJSON_CreateObject();
        cJSON_AddNumberToObject(comp, "number", g_config.compartments[i].number);
        cJSON_AddNumberToObject(comp, "servoPin", g_config.compartments[i].servoPin);
        cJSON_AddNumberToObject(comp, "limitOpenPin", g_config.compartments[i].limitOpenPin);
        cJSON_AddNumberToObject(comp, "limitClosePin", g_config.compartments[i].limitClosePin);
        cJSON_AddNumberToObject(comp, "ultrasonicTriggerPin", g_config.compartments[i].ultrasonicTriggerPin);
        cJSON_AddNumberToObject(comp, "ultrasonicEchoPin", g_config.compartments[i].ultrasonicEchoPin);
        cJSON_AddNumberToObject(comp, "weightSensorPin", g_config.compartments[i].weightSensorPin);
        cJSON_AddItemToArray(comps, comp);
    }
    // Serialize system section
    cJSON *sys = cJSON_AddObjectToObject(doc, "system");
    cJSON_AddNumberToObject(sys, "maxCompartments", g_config.system.maxCompartments);
    cJSON_AddBoolToObject(sys, "debugMode", g_config.system.debugMode);
    cJSON_AddNumberToObject(sys, "gracePeriodSec", g_config.system.gracePeriodSec);
    cJSON_AddNumberToObject(sys, "batteryLowThresholdPercent", g_config.system.batteryLowThresholdPercent);
    cJSON_AddNumberToObject(sys, "solarVoltageMin", g_config.system.solarVoltageMin);
    cJSON_AddNumberToObject(sys, "logLevel", g_config.system.logLevel);
    cJSON_AddStringToObject(sys, "otaPassword", g_config.system.otaPassword);
    // Serialize other section
    cJSON *oth = cJSON_AddObjectToObject(doc, "other");
    cJSON_AddNumberToObject(oth, "offlinePinTtlSec", g_config.other.offlinePinTtlSec);
    cJSON_AddNumberToObject(oth, "depositHoldAmountFallback", g_config.other.depositHoldAmountFallback);
    // Serialize to string
    char *jsonString = cJSON_Print(doc);
    vPortExitCritical(&g_configMutex);
    return jsonString;  // Caller must free with cJSON_free
}
