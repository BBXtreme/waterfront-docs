/**
 * @file config_loader.h
 * @brief Header for runtime configuration loading from LittleFS JSON.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Defines structs for all config and functions to load/save with validation.
 */

#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <vector>

// Structs for configuration
struct MqttConfig {
    std::string broker;
    int port;
    std::string username;
    std::string password;
};

struct WifiLteConfig {
    std::string apn;
    std::string simPin;
    int rssiThreshold;
};

struct LocationConfig {
    std::string slug;
    std::string code;
};

struct CompartmentConfig {
    int number;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
    int sensorPin;
};

// Global config struct (central place for ALL config)
struct Config {
    MqttConfig mqtt;
    WifiLteConfig wifiLte;
    LocationConfig location;
    std::vector<CompartmentConfig> compartments;
    int maxCompartments;
    bool debugMode;
    int gracePeriodSec;
};

// Global config instance
extern Config globalConfig;

// Function to load config from LittleFS, with validation and fallback
bool loadConfig();

// Function to save config to LittleFS (for remote update), with validation
bool saveConfig(const std::string& jsonStr);

// Function to get default config (static struct)
Config getDefaultConfig();

// Function to create default config.json file if missing
bool createDefaultConfigFile();

// Function to validate config (pins 0-39, etc.)
bool validateConfig(const Config& cfg);

#endif // CONFIG_LOADER_H
