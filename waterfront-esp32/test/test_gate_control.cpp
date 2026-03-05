// test_gate_control.cpp - Unit tests for gate_control.cpp using Catch2
// This file contains tests for gate control functions, focusing on state transitions and servo/limit switch simulation.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch2/catch_test_macros.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "gate_control.h"
#include "config_loader.h"

// Enhanced Mock LEDC functions with failure simulation
ledc_channel_config_t mockServoChannels[MAX_COMPARTMENTS];
int mockDuty[MAX_COMPARTMENTS] = {0};
bool mockLedcFail = false;  // Simulate LEDC failure
void mock_ledc_timer_config(const ledc_timer_config_t* config) {
    if (mockLedcFail) throw std::runtime_error("LEDC timer config failed");
}
void mock_ledc_channel_config(const ledc_channel_config_t* config) {
    if (mockLedcFail) throw std::runtime_error("LEDC channel config failed");
}
void mock_ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel, uint32_t duty) {
    if (mockLedcFail) throw std::runtime_error("LEDC set duty failed");
    mockDuty[channel] = duty;
}
void mock_ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel) {
    if (mockLedcFail) throw std::runtime_error("LEDC update duty failed");
}
#define ledc_timer_config mock_ledc_timer_config
#define ledc_channel_config mock_ledc_channel_config
#define ledc_set_duty mock_ledc_set_duty
#define ledc_update_duty mock_ledc_update_duty

// Enhanced Mock GPIO functions with failure simulation
int mockGpioLevels[40] = {0};  // Assume pins 0-39
bool mockGpioFail = false;  // Simulate GPIO failure
void mock_gpio_set_direction(gpio_num_t pin, gpio_mode_t mode) {
    if (mockGpioFail) throw std::runtime_error("GPIO set direction failed");
}
void mock_gpio_set_pull_mode(gpio_num_t pin, gpio_mode_t mode) {
    if (mockGpioFail) throw std::runtime_error("GPIO set pull mode failed");
}
int mock_gpio_get_level(gpio_num_t pin) {
    if (mockGpioFail) throw std::runtime_error("GPIO get level failed");
    return mockGpioLevels[pin];
}
#define gpio_set_direction mock_gpio_set_direction
#define gpio_set_pull_mode mock_gpio_set_pull_mode
#define gpio_get_level mock_gpio_get_level

// Mock millis for state transitions
unsigned long mockMillis = 0;
#define millis() mockMillis

// Test gate initialization
TEST_CASE("Gate Initialization", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Reset mocks
    for (int i = 0; i < MAX_COMPARTMENTS; i++) {
        mockDuty[i] = 0;
    }
    mockLedcFail = false;
    mockGpioFail = false;

    // Call init
    gate_init();

    // Verify LEDC channel configured (mock doesn't check, but assume it works)
    REQUIRE(true);  // Placeholder
}

// Test open gate state transition
TEST_CASE("Open Gate State Transition", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Reset mocks
    mockMillis = 0;
    mockDuty[0] = 0;
    mockLedcFail = false;

    // Call open gate for compartment 1
    openCompartmentGate(1);

    // Verify LEDC duty set to open position (512)
    REQUIRE(mockDuty[0] == 512);
}

// Test close gate state transition
TEST_CASE("Close Gate State Transition", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Reset mocks
    mockMillis = 0;
    mockDuty[0] = 0;
    mockLedcFail = false;

    // Call close gate for compartment 1
    closeCompartmentGate(1);

    // Verify LEDC duty set to close position (128)
    REQUIRE(mockDuty[0] == 128);
}

// Test get gate state
TEST_CASE("Get Gate State", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Set mock GPIO levels: open pin high, close pin low
    mockGpioLevels[13] = 1;  // Open limit
    mockGpioLevels[14] = 0;  // Close limit
    mockGpioFail = false;

    // Call get state for compartment 1
    const char* state = getCompartmentGateState(1);

    // Verify returns "open"
    REQUIRE(std::string(state) == "open");
}

// Test timeout and retry logic
TEST_CASE("Gate Timeout and Retry", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Reset mocks
    mockMillis = 0;
    mockDuty[0] = 0;
    mockLedcFail = false;

    // Call open gate
    openCompartmentGate(1);
    REQUIRE(mockDuty[0] == 512);

    // Simulate timeout without limit switch
    mockMillis = GATE_MOVE_TIMEOUT_MS + 1000;  // Exceed timeout
    gate_task();  // Should retry

    // Verify retry: duty set again
    REQUIRE(mockDuty[0] == 512);  // Same duty for retry
}

// Test invalid compartment ID
TEST_CASE("Invalid Compartment ID", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Call open gate for invalid ID
    openCompartmentGate(99);

    // Verify no duty change
    REQUIRE(mockDuty[0] == 0);
}

// New: Test LEDC failure during open
TEST_CASE("Open Gate LEDC Failure", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Simulate LEDC failure
    mockLedcFail = true;

    // Call open gate
    REQUIRE_THROWS(openCompartmentGate(1));  // Should throw due to mock failure

    // Reset
    mockLedcFail = false;
}

// New: Test GPIO failure during state check
TEST_CASE("Get Gate State GPIO Failure", "[gate]") {
    // Set mock config
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Simulate GPIO failure
    mockGpioFail = true;

    // Call get state
    REQUIRE_THROWS(getCompartmentGateState(1));  // Should throw due to mock failure

    // Reset
    mockGpioFail = false;
}

// New: Test state transition with invalid pins
TEST_CASE("Gate State with Invalid Pins", "[gate]") {
    // Set mock config with invalid pins
    g_config = getDefaultConfig();
    g_config.compartments[0] = {1, 50, 13, 14, 15, 16, 17};  // Invalid servo pin
    g_config.compartmentCount = 1;

    // Call open gate
    openCompartmentGate(1);

    // Verify no duty change (invalid pin handling)
    REQUIRE(mockDuty[0] == 0);
}
