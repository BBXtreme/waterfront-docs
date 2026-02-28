/**
 * @file compartment_manager.cpp
 * @brief Manages compartments using std::vector<Compartment> loaded from runtime config.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads compartment data from runtime config (LittleFS), provides access.
 */

// compartment_manager.cpp - Compartment management with std::vector
// This file defines a Compartment struct and manages a vector of compartments.
// Loads from runtime config; used for multi-compartment operations.
// Supports wildcards for MQTT subscriptions to test multi-compartment handling.

#include "compartment_manager.h"
#include "config_loader.h"
#include <vector>
#include <string>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Compartment struct (updated to match config)
struct Compartment {
    int id;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
    int sensorPin;
    std::string name;
};

// Vector of compartments
std::vector<Compartment> compartments;

// Load compartments from runtime config
void load_compartments() {
    compartments.clear();
    for (const auto& comp : globalConfig.compartments) {
        Compartment c;
        c.id = comp.number;
        c.servoPin = comp.servoPin;
        c.limitOpenPin = comp.limitOpenPin;
        c.limitClosePin = comp.limitClosePin;
        c.sensorPin = comp.sensorPin;
        c.name = "Compartment " + std::to_string(c.id);
        compartments.push_back(c);
    }
    if (compartments.empty()) {
        ESP_LOGW("COMPARTMENT", "No compartments in config, using defaults");
        compartments.push_back({1, SERVO_PIN_1, 13, 14, 18, "Compartment 1"});
        compartments.push_back({2, SERVO_PIN_2, 16, 17, 19, "Compartment 2"});
        compartments.push_back({3, SERVO_PIN_3, 20, 21, 22, "Compartment 3"});
    }
    ESP_LOGI("COMPARTMENT", "Loaded %d compartments from config", compartments.size());
}

// Get compartment by ID
Compartment* get_compartment(int id) {
    for (auto& comp : compartments) {
        if (comp.id == id) {
            return &comp;
        }
    }
    return nullptr;
}

// Get all compartments (for iteration)
std::vector<Compartment>& get_all_compartments() {
    return compartments;
}

// Initialize compartment manager
void compartment_init() {
    load_compartments();
    ESP_LOGI("COMPARTMENT", "Initialized with %d compartments", compartments.size());
}
