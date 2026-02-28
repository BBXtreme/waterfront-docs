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

struct BleConfig {
    String serviceUuid;
    String ssidCharUuid;
    String passCharUuid;
    String statusCharUuid;
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
    BleConfig ble;
    std::vector<CompartmentConfig> compartments;
    SystemConfig system;
    OtherConfig other;
};

// Global config instance
extern GlobalConfig g_config;

// Function to load config from LittleFS, with validation and fallback
bool loadConfig();

#endif // CONFIG_LOADER_H
