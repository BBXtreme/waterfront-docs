/**
 * @file compartment_manager.cpp
 * @brief Manages compartments using std::vector<Compartment> loaded from config.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Loads compartment data from LittleFS config.json and provides access.
 */

// compartment_manager.cpp - Compartment management with std::vector
// This file defines a Compartment struct and manages a vector of compartments.
// Loads from config.json on startup; used for multi-compartment operations.
// Supports wildcards for MQTT subscriptions to test multi-compartment handling.

#include "compartment_manager.h"
#include "config.h"
#include <vector>
#include <string>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Compartment struct
struct Compartment {
    int id;
    int pin;
    std::string name;
};

// Vector of compartments
std::vector<Compartment> compartments;

// Load compartments from config.json
void load_compartments() {
    if (!LittleFS.begin()) {
        ESP_LOGE("COMPARTMENT", "Failed to mount LittleFS");
        return;
    }
    File configFile = LittleFS.open("/config.json", "r");
    if (!configFile) {
        ESP_LOGW("COMPARTMENT", "config.json not found, using defaults");
        // Add default compartments
        compartments.push_back({1, SERVO_PIN_1, "Compartment 1"});
        compartments.push_back({2, SERVO_PIN_2, "Compartment 2"});
        compartments.push_back({3, SERVO_PIN_3, "Compartment 3"});
        return;
    }
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        ESP_LOGE("COMPARTMENT", "Failed to parse config.json: %s", error.c_str());
        configFile.close();
        return;
    }
    configFile.close();
    // Load compartments array
    if (doc.containsKey("compartments")) {
        JsonArray comps = doc["compartments"];
        compartments.clear();
        for (JsonObject comp : comps) {
            Compartment c;
            c.id = comp["id"];
            c.pin = comp["pin"];
            c.name = comp["name"].as<std::string>();
            compartments.push_back(c);
        }
        ESP_LOGI("COMPARTMENT", "Loaded %d compartments from config", compartments.size());
    } else {
        ESP_LOGW("COMPARTMENT", "No compartments in config, using defaults");
        compartments.push_back({1, SERVO_PIN_1, "Compartment 1"});
        compartments.push_back({2, SERVO_PIN_2, "Compartment 2"});
        compartments.push_back({3, SERVO_PIN_3, "Compartment 3"});
    }
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
```