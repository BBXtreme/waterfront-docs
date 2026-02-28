// gate_control.cpp - Gate control logic for servo and limit switches
// This file handles physical gate operations for kayak slots.
// Uses servo for gate movement and limit switches for feedback.
// Integrates with MQTT for real-time control.

#include "gate_control.h"
#include "config.h"
#include <Servo.h>

// Servo pins (adjust per slot; example for slot 1)
#define SERVO_PIN_1 12  // GPIO for servo on slot 1
#define LIMIT_OPEN_PIN_1 13  // Limit switch for open position
#define LIMIT_CLOSE_PIN_1 14  // Limit switch for closed position

// Servo objects (one per slot)
Servo servo1;  // Add more for additional slots

// Initialize gate control
void gate_init() {
    servo1.attach(SERVO_PIN_1);
    pinMode(LIMIT_OPEN_PIN_1, INPUT_PULLUP);
    pinMode(LIMIT_CLOSE_PIN_1, INPUT_PULLUP);
    closeGateServo(1);  // Start closed
    ESP_LOGI("GATE", "Initialized for slot 1");
}

// Open gate for slot
void openGateServo(int slotId) {
    if (slotId == 1) {
        servo1.write(90);  // Example angle for open; adjust
        delay(1000);  // Wait for movement
        while (!digitalRead(LIMIT_OPEN_PIN_1)) {
            // Wait for limit switch
            delay(100);
        }
        ESP_LOGI("GATE", "Slot %d gate opened at %lu", slotId, millis());
    }
    // Add logic for other slots
}

// Close gate for slot
void closeGateServo(int slotId) {
    if (slotId == 1) {
        servo1.write(0);  // Example angle for close; adjust
        delay(1000);
        while (!digitalRead(LIMIT_CLOSE_PIN_1)) {
            delay(100);
        }
        ESP_LOGI("GATE", "Slot %d gate closed at %lu", slotId, millis());
    }
    // Add logic for other slots
}

// Get gate state (open/closed)
const char* getGateState(int slotId) {
    if (slotId == 1) {
        if (digitalRead(LIMIT_OPEN_PIN_1)) return "open";
        if (digitalRead(LIMIT_CLOSE_PIN_1)) return "closed";
    }
    return "unknown";
}
