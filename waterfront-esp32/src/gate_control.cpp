// gate_control.cpp - Gate control logic for servo and limit switches
// This file handles physical gate operations for kayak compartments.
// Uses servo for gate movement and limit switches for feedback.
// Integrates with MQTT for real-time control.
// Non-blocking: uses millis() state machine for servo control.

#include "gate_control.h"
#include "config.h"
#include <Servo.h>

// Pin definitions for compartments (adjust in config.h if needed)
#define SERVO_PIN_1 12  // GPIO for servo on compartment 1
#define LIMIT_OPEN_PIN_1 13  // Limit switch for open position
#define LIMIT_CLOSE_PIN_1 14  // Limit switch for closed position
// Add more for additional compartments as needed

// Servo objects (array for MAX_COMPARTMENTS)
#define MAX_COMPARTMENTS 10
Servo servos[MAX_COMPARTMENTS];

// State machine for each compartment
enum GateState { IDLE, OPENING, CLOSING };
GateState gateStates[MAX_COMPARTMENTS] = {IDLE};
unsigned long gateStartTimes[MAX_COMPARTMENTS] = {0};
#define GATE_MOVE_DURATION_MS 2000  // Time to move gate

// Initialize gate control
void gate_init() {
    // Initialize pins and servos for active compartments
    for (int i = 0; i < MAX_COMPARTMENTS; i++) {
        if (i == 0) {  // Compartment 1
            pinMode(LIMIT_OPEN_PIN_1, INPUT_PULLUP);
            pinMode(LIMIT_CLOSE_PIN_1, INPUT_PULLUP);
            servos[i].attach(SERVO_PIN_1);
            closeCompartmentGate(i + 1);  // Start closed
        }
        // Add more compartments as needed
    }
    ESP_LOGI("GATE", "Initialized for %d compartments", MAX_COMPARTMENTS);
}

// Open gate for compartment (non-blocking)
void openCompartmentGate(int compartmentId) {
    if (compartmentId < 1 || compartmentId > MAX_COMPARTMENTS) return;
    int idx = compartmentId - 1;
    if (gateStates[idx] == IDLE) {
        gateStates[idx] = OPENING;
        gateStartTimes[idx] = millis();
        servos[idx].write(90);  // Example angle for open; adjust
        ESP_LOGI("GATE", "Starting open for compartment %d", compartmentId);
    }
}

// Close gate for compartment (non-blocking)
void closeCompartmentGate(int compartmentId) {
    if (compartmentId < 1 || compartmentId > MAX_COMPARTMENTS) return;
    int idx = compartmentId - 1;
    if (gateStates[idx] == IDLE) {
        gateStates[idx] = CLOSING;
        gateStartTimes[idx] = millis();
        servos[idx].write(0);  // Example angle for close; adjust
        ESP_LOGI("GATE", "Starting close for compartment %d", compartmentId);
    }
}

// Get gate state
const char* getCompartmentGateState(int compartmentId) {
    if (compartmentId < 1 || compartmentId > MAX_COMPARTMENTS) return "unknown";
    int idx = compartmentId - 1;
    if (idx == 0) {  // Compartment 1
        if (digitalRead(LIMIT_OPEN_PIN_1)) return "open";
        if (digitalRead(LIMIT_CLOSE_PIN_1)) return "closed";
    }
    return "unknown";
}

// Task to handle servo movement (call from main loop)
void gate_task() {
    unsigned long now = millis();
    for (int i = 0; i < MAX_COMPARTMENTS; i++) {
        if (gateStates[i] != IDLE && (now - gateStartTimes[i]) >= GATE_MOVE_DURATION_MS) {
            // Movement complete, check limit switches
            if (gateStates[i] == OPENING) {
                if (i == 0 && digitalRead(LIMIT_OPEN_PIN_1)) {
                    ESP_LOGI("GATE", "Compartment %d gate opened", i + 1);
                }
            } else if (gateStates[i] == CLOSING) {
                if (i == 0 && digitalRead(LIMIT_CLOSE_PIN_1)) {
                    ESP_LOGI("GATE", "Compartment %d gate closed", i + 1);
                }
            }
            gateStates[i] = IDLE;
        }
    }
}
