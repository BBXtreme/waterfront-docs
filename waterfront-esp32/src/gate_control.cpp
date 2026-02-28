// gate_control.cpp - Gate control logic for servo and limit switches
// This file handles physical gate operations for kayak slots.
// Uses servo for gate movement and limit switches for feedback.
// Integrates with MQTT for real-time control.
// Non-blocking: uses millis() state machine for servo control.

#include "gate_control.h"
#include "config.h"
#include <Servo.h>

// Pin definitions for slots (adjust in config.h if needed)
#define SERVO_PIN_1 12  // GPIO for servo on slot 1
#define LIMIT_OPEN_PIN_1 13  // Limit switch for open position
#define LIMIT_CLOSE_PIN_1 14  // Limit switch for closed position
// Add more for additional slots as needed

// Servo objects (array for MAX_SLOTS)
#define MAX_SLOTS 10
Servo servos[MAX_SLOTS];

// State machine for each slot
enum GateState { IDLE, OPENING, CLOSING };
GateState gateStates[MAX_SLOTS] = {IDLE};
unsigned long gateStartTimes[MAX_SLOTS] = {0};
#define GATE_MOVE_DURATION_MS 2000  // Time to move gate

// Initialize gate control
void gate_init() {
    // Initialize pins and servos for active slots
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (i == 0) {  // Slot 1
            pinMode(LIMIT_OPEN_PIN_1, INPUT_PULLUP);
            pinMode(LIMIT_CLOSE_PIN_1, INPUT_PULLUP);
            servos[i].attach(SERVO_PIN_1);
            closeGateServo(i + 1);  // Start closed
        }
        // Add more slots as needed
    }
    ESP_LOGI("GATE", "Initialized for %d slots", MAX_SLOTS);
}

// Open gate for slot (non-blocking)
void openGateServo(int slotId) {
    if (slotId < 1 || slotId > MAX_SLOTS) return;
    int idx = slotId - 1;
    if (gateStates[idx] == IDLE) {
        gateStates[idx] = OPENING;
        gateStartTimes[idx] = millis();
        servos[idx].write(90);  // Example angle for open; adjust
        ESP_LOGI("GATE", "Starting open for slot %d", slotId);
    }
}

// Close gate for slot (non-blocking)
void closeGateServo(int slotId) {
    if (slotId < 1 || slotId > MAX_SLOTS) return;
    int idx = slotId - 1;
    if (gateStates[idx] == IDLE) {
        gateStates[idx] = CLOSING;
        gateStartTimes[idx] = millis();
        servos[idx].write(0);  // Example angle for close; adjust
        ESP_LOGI("GATE", "Starting close for slot %d", slotId);
    }
}

// Get gate state
const char* getGateState(int slotId) {
    if (slotId < 1 || slotId > MAX_SLOTS) return "unknown";
    int idx = slotId - 1;
    if (idx == 0) {  // Slot 1
        if (digitalRead(LIMIT_OPEN_PIN_1)) return "open";
        if (digitalRead(LIMIT_CLOSE_PIN_1)) return "closed";
    }
    return "unknown";
}

// Task to handle servo movement (call from main loop)
void gate_task() {
    unsigned long now = millis();
    for (int i = 0; i < MAX_SLOTS; i++) {
        if (gateStates[i] != IDLE && (now - gateStartTimes[i]) >= GATE_MOVE_DURATION_MS) {
            // Movement complete, check limit switches
            if (gateStates[i] == OPENING) {
                if (i == 0 && digitalRead(LIMIT_OPEN_PIN_1)) {
                    ESP_LOGI("GATE", "Slot %d gate opened", i + 1);
                }
            } else if (gateStates[i] == CLOSING) {
                if (i == 0 && digitalRead(LIMIT_CLOSE_PIN_1)) {
                    ESP_LOGI("GATE", "Slot %d gate closed", i + 1);
                }
            }
            gateStates[i] = IDLE;
        }
    }
}
