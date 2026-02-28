// deposit_logic.cpp - Deposit hold/release logic for WATERFRONT
// This file manages deposit states based on booking and sensor events.
// It handles hold on take, release on timely return, and auto-charge on overdue.
// Integrates with MQTT for real-time sync and gate control.

#include "deposit_logic.h"
#include "config_loader.h"
#include "gate_control.h"
#include "mqtt_handler.h"
#include <PubSubClient.h>

// External MQTT client
extern PubSubClient mqttClient;

// Vector of active timers
std::vector<RentalTimer> activeTimers;

// Current booking state
static bool deposit_held = false;
static unsigned long rental_start_time = 0;
static unsigned long rental_duration_ms = 0;

// Initialize deposit logic
void deposit_init() {
    deposit_held = false;
    ESP_LOGI("DEPOSIT", "Initialized");
}

// Start a rental timer for a compartment
void startRental(int compartmentId, unsigned long durationSec) {
    RentalTimer timer;
    timer.compartmentId = compartmentId;
    timer.startMs = millis();
    timer.durationSec = durationSec;
    activeTimers.push_back(timer);
    ESP_LOGI("DEPOSIT", "Started rental timer for compartment %d, duration %lu sec", compartmentId, durationSec);
}

// Check for overdue rentals and auto-lock
void checkOverdue() {
    unsigned long now = millis();
    for (auto it = activeTimers.begin(); it != activeTimers.end(); ) {
        unsigned long elapsedSec = (now - it->startMs) / 1000;
        unsigned long totalAllowedSec = it->durationSec + g_config.system.gracePeriodSec;
        if (elapsedSec > totalAllowedSec) {
            // Overdue: auto-lock
            closeCompartmentGate(it->compartmentId);
            // Publish overdue event
            char topic[96];
            snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/%d/event", g_config.location.slug.c_str(), g_config.location.code.c_str(), it->compartmentId);
            char payload[128];
            snprintf(payload, sizeof(payload), "{\"event\":\"overdue\",\"compartmentId\":%d,\"elapsedSec\":%lu}", it->compartmentId, elapsedSec);
            mqttClient.publish(topic, payload);
            ESP_LOGW("DEPOSIT", "Compartment %d overdue, auto-locked", it->compartmentId);
            // Remove timer
            it = activeTimers.erase(it);
        } else {
            ++it;
        }
    }
}

// On kayak taken: hold deposit, start timer
void deposit_on_take() {
    deposit_held = true;
    rental_start_time = millis();
    rental_duration_ms = g_config.system.gracePeriodSec * 1000;
    ESP_LOGI("DEPOSIT", "Deposit held, rental started");
}

// On kayak returned: check timing, release deposit if on time
void deposit_on_return(PubSubClient* client) {
    if (!deposit_held) return;
    unsigned long elapsed = millis() - rental_start_time;
    if (elapsed <= rental_duration_ms) {
        // On-time return: release deposit
        deposit_held = false;
        ESP_LOGI("DEPOSIT", "Deposit released (on-time return)");
        // Publish deposit release event
        char topic[64];
        char payload[128];
        snprintf(topic, sizeof(topic), "waterfront/locations/%s/depositRelease", g_config.location.code.c_str());
        snprintf(payload, sizeof(payload), "{\"bookingId\":\"current\",\"release\":true}");
        client->publish(topic, payload);
    } else {
        // Overdue: keep held, publish for admin action
        ESP_LOGW("DEPOSIT", "Overdue return, deposit held");
        // TODO: Publish overdue event or charge via MQTT
    }
}

// Check for overdue (call periodically)
void deposit_check_overdue(PubSubClient* client) {
    if (!deposit_held) return;
    unsigned long elapsed = millis() - rental_start_time;
    if (elapsed > rental_duration_ms + 3600000) {  // 1 hour grace
        // Auto-lock or alert
        ESP_LOGW("DEPOSIT", "Rental overdue, triggering gate control");
        // Publish returnConfirm for auto-lock
        char topic[64];
        char payload[128];
        snprintf(topic, sizeof(topic), "waterfront/locations/%s/returnConfirm", g_config.location.code.c_str());
        snprintf(payload, sizeof(payload), "{\"bookingId\":\"current\",\"action\":\"autoLock\"}");
        client->publish(topic, payload);
    }
}

// Get deposit status
bool deposit_is_held() {
    return deposit_held;
}
