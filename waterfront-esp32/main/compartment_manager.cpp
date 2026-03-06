/**
 * @file compartment_manager.cpp
 * @brief Manages compartments using std::vector<Compartment> loaded from runtime config.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads compartment data from runtime config, provides access.
 */

// compartment_manager.cpp - Compartment management with std::vector
// This file defines a Compartment struct and manages a vector of compartments.
// Loads from runtime config; used for multi-compartment operations.
// Supports wildcards for MQTT subscriptions to test multi-compartment handling.

#include "compartment_manager.h"
#include "config_loader.h"
#include <esp_vfs.h>
#include <esp_littlefs.h>
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

// LittleFS configuration
static esp_littlefs_config_t littlefs_config = {
    .base_path = "/littlefs",
    .partition_label = NULL,
    .format_if_mount_failed = true,
    .dont_mount = false
};

// Compartment struct (updated to match config)
struct Compartment {
    int id;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
    std::string name;
};

// Vector of compartments
std::vector<Compartment> compartments;

// Mutex for thread-safe access to compartments
portMUX_TYPE compartmentMutex = portMUX_INITIALIZER_UNLOCKED;

// Load compartments from runtime config
void load_compartments() {
    vPortEnterCritical(&compartmentMutex);
    compartments.clear();
    for (int i = 0; i < g_config.compartmentCount; i++) {
        Compartment c;
        c.id = g_config.compartments[i].number;
        c.servoPin = g_config.compartments[i].servoPin;
        c.limitOpenPin = g_config.compartments[i].limitOpenPin;
        c.limitClosePin = g_config.compartments[i].limitClosePin;
        c.name = "Compartment " + std::to_string(c.id);
        compartments.push_back(c);
    }
    if (compartments.empty()) {
        ESP_LOGW("COMPARTMENT", "No compartments in config, using defaults");
        compartments.push_back({1, 12, 13, 14, "Compartment 1"});
    }
    ESP_LOGI("COMPARTMENT", "Loaded %d compartments from config", compartments.size());
    vPortExitCritical(&compartmentMutex);
}

// Get compartment by ID
Compartment* get_compartment(int id) {
    vPortEnterCritical(&compartmentMutex);
    for (auto& comp : compartments) {
        if (comp.id == id) {
            Compartment* result = &comp;
            vPortExitCritical(&compartmentMutex);
            return result;
        }
    }
    vPortExitCritical(&compartmentMutex);
    return nullptr;
}

// Get all compartments (for iteration)
std::vector<Compartment>& get_all_compartments() {
    // Note: Caller must handle mutex if modifying the vector
    return compartments;
}

// Initialize compartment manager
void compartment_init() {
    load_compartments();
    ESP_LOGI("COMPARTMENT", "Initialized with %d compartments", compartments.size());
}
