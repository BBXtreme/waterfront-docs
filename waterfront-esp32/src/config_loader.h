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
};

struct Location {
    String slug;
    String code;
};

struct CompartmentPin {
    int number;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
};

// Global config struct (central place for ALL config)
struct GlobalConfig {
    Mqtt mqtt;
    Location location;
    int maxCompartments;
    bool debugMode;
    std::vector<CompartmentPin> compartments;
};

// Global config instance
extern GlobalConfig g_config;

// Function to load config from LittleFS, with validation and fallback
bool loadConfig();

#endif // CONFIG_LOADER_H
