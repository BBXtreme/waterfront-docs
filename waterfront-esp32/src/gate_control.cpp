// gate_control.cpp - Gate control logic for servo and limit switches
// This file handles physical gate operations for kayak compartments.
// Uses servo for gate movement and limit switches for feedback.
// Integrates with MQTT for real-time control.
// Non-blocking: uses millis() state machine for servo control.
// Added state machine with timeouts for stuck gates.

#include "gate_control.h"
#include "config_loader.h"
#include <driver/gpio.h>
#include <driver/ledc.h>

// Pin definitions for compartments (adjust in config.h if needed)
// Note: Pins are now loaded from g_config.compartments[idx].servoPin, etc.

// LEDC channels for servos (array for MAX_COMPARTMENTS)
#define MAX_COMPARTMENTS 10
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT
#define LEDC_FREQUENCY 50  // 50 Hz for servos
ledc_channel_config_t servo_channels[MAX_COMPARTMENTS];

// State machine for each compartment
enum CompartmentState { IDLE, OPENING, CLOSING, OPEN, CLOSED, ERROR };
CompartmentState compartmentStates[MAX_COMPARTMENTS] = {CLOSED};
unsigned long compartmentStartTimes[MAX_COMPARTMENTS] = {0};
#define GATE_MOVE_TIMEOUT_MS 10000  // 10 seconds timeout for stuck gates
#define RETRY_ATTEMPTS 3  // Max retries before error

int retryCounts[MAX_COMPARTMENTS] = {0};

// Mutex for thread-safe access to gate control state
portMUX_TYPE gateMutex = portMUX_INITIALIZER_UNLOCKED;

/**
 * @brief Initializes gate control for all compartments.
 * @note Sets up pins and servos for active compartments.
 */
void gate_init() {
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Initialize pins and servos for active compartments
    for (int i = 0; i < g_config.compartmentCount; i++) {
        if (i < MAX_COMPARTMENTS) {
            gpio_set_direction((gpio_num_t)g_config.compartments[i].limitOpenPin, GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)g_config.compartments[i].limitOpenPin, GPIO_PULLUP_ONLY);
            gpio_set_direction((gpio_num_t)g_config.compartments[i].limitClosePin, GPIO_MODE_INPUT);
            gpio_set_pull_mode((gpio_num_t)g_config.compartments[i].limitClosePin, GPIO_PULLUP_ONLY);

            // Configure LEDC channel for servo
            servo_channels[i].gpio_num = g_config.compartments[i].servoPin;
            servo_channels[i].speed_mode = LEDC_MODE;
            servo_channels[i].channel = (ledc_channel_t)i;
            servo_channels[i].intr_type = LEDC_INTR_DISABLE;
            servo_channels[i].timer_sel = LEDC_TIMER;
            servo_channels[i].duty = 0;
            servo_channels[i].hpoint = 0;
            ledc_channel_config(&servo_channels[i]);

            compartmentStates[i] = CLOSED;  // Start closed
        }
    }
    ESP_LOGI("GATE", "Initialized for %d compartments", g_config.compartmentCount);
}

/**
 * @brief Opens gate for compartment (non-blocking).
 * @param compartmentId The compartment ID to open.
 * @note Sets state to OPENING and starts servo movement.
 */
void openCompartmentGate(int compartmentId) {
    if (compartmentId < 1 || compartmentId > g_config.compartmentCount) return;
    int idx = compartmentId - 1;
    vPortEnterCritical(&gateMutex);
    if (compartmentStates[idx] == CLOSED || compartmentStates[idx] == ERROR) {
        compartmentStates[idx] = OPENING;
        compartmentStartTimes[idx] = esp_timer_get_time() / 1000;
        // Set servo to open position (adjust duty cycle for angle)
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)idx, 512);  // Example duty for 90 degrees; adjust
        ledc_update_duty(LEDC_MODE, (ledc_channel_t)idx);
        retryCounts[idx] = 0;
        ESP_LOGI("GATE", "Starting open for compartment %d", compartmentId);
    }
    vPortExitCritical(&gateMutex);
}

/**
 * @brief Closes gate for compartment (non-blocking).
 * @param compartmentId The compartment ID to close.
 * @note Sets state to CLOSING and starts servo movement.
 */
void closeCompartmentGate(int compartmentId) {
    if (compartmentId < 1 || compartmentId > g_config.compartmentCount) return;
    int idx = compartmentId - 1;
    vPortEnterCritical(&gateMutex);
    if (compartmentStates[idx] == OPEN || compartmentStates[idx] == ERROR) {
        compartmentStates[idx] = CLOSING;
        compartmentStartTimes[idx] = esp_timer_get_time() / 1000;
        // Set servo to close position (adjust duty cycle for angle)
        ledc_set_duty(LEDC_MODE, (ledc_channel_t)idx, 128);  // Example duty for 0 degrees; adjust
        ledc_update_duty(LEDC_MODE, (ledc_channel_t)idx);
        retryCounts[idx] = 0;
        ESP_LOGI("GATE", "Starting close for compartment %d", compartmentId);
    }
    vPortExitCritical(&gateMutex);
}

/**
 * @brief Gets gate state for compartment.
 * @param compartmentId The compartment ID.
 * @return "open", "closed", or "unknown".
 * @note Checks limit switches for compartment 1.
 */
const char* getCompartmentGateState(int compartmentId) {
    if (compartmentId < 1 || compartmentId > g_config.compartmentCount) return "unknown";
    int idx = compartmentId - 1;
    vPortEnterCritical(&gateMutex);
    if (gpio_get_level((gpio_num_t)g_config.compartments[idx].limitOpenPin) == 0) {
        vPortExitCritical(&gateMutex);
        return "open";
    }
    if (gpio_get_level((gpio_num_t)g_config.compartments[idx].limitClosePin) == 0) {
        vPortExitCritical(&gateMutex);
        return "closed";
    }
    vPortExitCritical(&gateMutex);
    return "unknown";
}

/**
 * @brief Task to handle servo movement and timeouts (call from main loop).
 * @note Checks if movement duration has passed, updates state, handles retries on timeout.
 */
void gate_task() {
    unsigned long now = esp_timer_get_time() / 1000;
    for (int i = 0; i < g_config.compartmentCount; i++) {
        vPortEnterCritical(&gateMutex);
        if (compartmentStates[i] == OPENING || compartmentStates[i] == CLOSING) {
            if (now - compartmentStartTimes[i] > GATE_MOVE_TIMEOUT_MS) {
                // Timeout: retry or error
                if (retryCounts[i] < RETRY_ATTEMPTS) {
                    retryCounts[i]++;
                    ESP_LOGW("GATE", "Compartment %d stuck, retrying (attempt %d)", i + 1, retryCounts[i]);
                    if (compartmentStates[i] == OPENING) {
                        openCompartmentGate(i + 1);
                    } else {
                        closeCompartmentGate(i + 1);
                    }
                } else {
                    compartmentStates[i] = ERROR;
                    ESP_LOGE("GATE", "Compartment %d error: stuck after %d retries", i + 1, RETRY_ATTEMPTS);
                    // TODO: Publish MQTT alert or trigger admin notification
                }
            } else {
                // Check limit switches for completion
                if (compartmentStates[i] == OPENING) {
                    if (gpio_get_level((gpio_num_t)g_config.compartments[i].limitOpenPin) == 0) {
                        compartmentStates[i] = OPEN;
                        ESP_LOGI("GATE", "Compartment %d gate opened", i + 1);
                    }
                } else if (compartmentStates[i] == CLOSING) {
                    if (gpio_get_level((gpio_num_t)g_config.compartments[i].limitClosePin) == 0) {
                        compartmentStates[i] = CLOSED;
                        ESP_LOGI("GATE", "Compartment %d gate closed", i + 1);
                    }
                }
            }
        }
        vPortExitCritical(&gateMutex);
    }
}
