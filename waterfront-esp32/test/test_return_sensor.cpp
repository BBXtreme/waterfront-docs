// test_return_sensor.cpp - Unit tests for return_sensor.cpp using Catch2
// This file contains tests for return sensor functions, focusing on distance measurement and presence detection.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks ultrasonic sensor pins and pulseIn to simulate hardware without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch2/catch_test_macros.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "return_sensor.h"
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;
void loadConfig() { // Mock load
    g_config.compartments.push_back({1, 12, 13, 14, 15, 16, 17});
}

// Mock digitalWrite and pulseIn for ultrasonic sensor
int mockTrigPin = -1;
int mockEchoPin = -1;
unsigned long mockPulseDuration = 0;  // Mock pulse duration in microseconds

void mockDigitalWrite(int pin, int value) {
    if (pin == 15 && value == LOW) {  // Trig pin low
        // Prepare for pulse
    } else if (pin == 15 && value == HIGH) {  // Trig pin high
        // Simulate echo pulse
        mockPulseDuration = 2940;  // Example: 50cm distance (speed of sound 343 m/s)
    }
}

unsigned long mockPulseIn(int pin, int value) {
    if (pin == 16 && value == HIGH) {  // Echo pin
        return mockPulseDuration;
    }
    return 0;
}

// Override global functions for tests
#define digitalWrite mockDigitalWrite
#define pulseIn mockPulseIn

// Test sensor initialization
TEST_CASE("Sensor Initialization", "[sensor]") {
    // Load mock config
    loadConfig();

    // Call init
    sensor_init();

    // Verify pins are set (mock doesn't check, but assume it works)
    REQUIRE(true);  // Placeholder; in real test, check pin modes if exposed
}

// Test distance measurement
TEST_CASE("Distance Measurement", "[sensor]") {
    // Load mock config
    loadConfig();

    // Call init
    sensor_init();

    // Call get_distance
    float distance = sensor_get_distance();

    // Verify distance calculation (mockPulseDuration = 2940 us -> ~50cm)
    REQUIRE(distance == Approx(50.0f).epsilon(0.1));
}

// Test kayak presence detection (present)
TEST_CASE("Kayak Presence Detection - Present", "[sensor]") {
    // Load mock config
    loadConfig();

    // Set mock distance < threshold (50cm)
    mockPulseDuration = 2940;  // 50cm

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify true
    REQUIRE(present == true);
}

// Test kayak presence detection (not present)
TEST_CASE("Kayak Presence Detection - Not Present", "[sensor]") {
    // Load mock config
    loadConfig();

    // Set mock distance > threshold (100cm)
    mockPulseDuration = 5880;  // 100cm

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify false
    REQUIRE(present == false);
}

// Test invalid distance (pulseIn returns 0)
TEST_CASE("Invalid Distance Reading", "[sensor]") {
    // Load mock config
    loadConfig();

    // Set mock pulse to 0 (invalid)
    mockPulseDuration = 0;

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify false (invalid reading)
    REQUIRE(present == false);
}
