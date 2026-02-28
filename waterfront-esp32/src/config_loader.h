/**
 * @file config_loader.h
 * @brief Header for runtime configuration loading from LittleFS JSON.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Defines structs for all config and functions to load/save with validation.
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <Arduino.h>

// Structs for configuration
struct Mqtt {
    String broker;
    int port;
    String username;
    String password;
    String clientIdPrefix;
    bool useTLS;
};

struct Location {
    String slug;
    String code;
};

struct WifiProvisioningConfig {
    String fallbackSsid;
    String fallbackPass;
};

struct LteConfig {
    String apn;
    String simPin;
    int rssiThreshold;
    int dataUsageAlertLimitKb;
};

struct CompartmentConfig {
    int number;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
    int ultrasonicTriggerPin;
    int ultrasonicEchoPin;
    int weightSensorPin;
};

struct SystemConfig {
    int maxCompartments;
    bool debugMode;
    int gracePeriodSec;
    int batteryLowThresholdPercent;
    float solarVoltageMin;
};

struct OtherConfig {
    int offlinePinTtlSec;
    int depositHoldAmountFallback;
};

// Global config struct (central place for ALL config)
struct GlobalConfig {
    Mqtt mqtt;
    Location location;
    WifiProvisioningConfig wifiProvisioning;
    LteConfig lte;
    std::vector<CompartmentConfig> compartments;
    SystemConfig system;
    OtherConfig other;
};

// Global config instance
extern GlobalConfig g_config;

// Function to load config from LittleFS, with validation and fallback
bool loadConfig();

// Function to save config to LittleFS (for remote update), with validation
bool saveConfig();

// Function to update config from JSON payload, validate, save, reload
bool updateConfigFromJson(const std::string& jsonPayload);

// Function to get default config (static struct)
GlobalConfig getDefaultConfig();

// Function to validate config (pins 0-39, etc.)
bool validateConfig(const GlobalConfig& cfg);

#endif // CONFIG_LOADER_H
