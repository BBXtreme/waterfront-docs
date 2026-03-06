// relay_handler.cpp - Relay control for lock mechanism in WATERFRONT ESP32
// This file handles the solenoid relay for unlocking/locking the kayak compartment.
// Uses GPIO pulses for safe operation.

#include "relay_handler.h"
#include "config.h"


// Initialize relay pin
void relay_init() {
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);  // Start locked
    ESP_LOGI("RELAY", "Initialized on GPIO %d", RELAY_PIN);
}

// Unlock the relay (pulse high)
void relay_unlock() {
    digitalWrite(RELAY_PIN, HIGH);
    delay(RELAY_PULSE_DURATION_MS);
    digitalWrite(RELAY_PIN, LOW);
    ESP_LOGI("RELAY", "Unlocked for %d ms", RELAY_PULSE_DURATION_MS);
}

// Lock the relay (ensure low)
void relay_lock() {
    digitalWrite(RELAY_PIN, LOW);
    ESP_LOGI("RELAY", "Locked");
}

// Get relay state (high = unlocked, low = locked)
bool relay_is_unlocked() {
    return digitalRead(RELAY_PIN) == HIGH;
}

// Emergency lock (force low)
void relay_emergency_lock() {
    digitalWrite(RELAY_PIN, LOW);
    ESP_LOGW("RELAY", "Emergency lock activated");
}
