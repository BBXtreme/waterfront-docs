// test_performance.cpp - Performance tests for critical functions
// Measures execution time to ensure operations stay under thresholds.
// Uses Catch2 and std::chrono for timing.
// Run with PlatformIO: pio test -e unit_test --filter "*Performance*"

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>
#include <chrono>

// Include headers under test
#include "gate_control.h"
#include "config_loader.h"
#include "return_sensor.h"

// Define global config for tests
GlobalConfig g_config;

// Mock millis for performance tests
unsigned long mockMillis = 0;
#define millis() mockMillis

// Mock GPIO for sensor
int mockGpioLevels[40] = {0};
int mock_gpio_get_level(gpio_num_t pin) { return mockGpioLevels[pin]; }
#define gpio_get_level mock_gpio_get_level

// Mock LEDC
int mockDuty[MAX_COMPARTMENTS] = {0};
void mock_ledc_set_duty(ledc_mode_t mode, ledc_channel_t channel, uint32_t duty) { mockDuty[channel] = duty; }
void mock_ledc_update_duty(ledc_mode_t mode, ledc_channel_t channel) {}
#define ledc_set_duty mock_ledc_set_duty
#define ledc_update_duty mock_ledc_update_duty

// Mock esp_timer_get_time
uint64_t mockEspTimer = 0;
uint64_t esp_timer_get_time() { return mockEspTimer; }

// Performance test for gate_task (should complete <50ms)
TEST_CASE("Gate Task Performance", "[performance]") {
    // Setup config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    gate_init();

    // Measure time
    auto start = std::chrono::high_resolution_clock::now();
    gate_task();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    REQUIRE(duration.count() < 50);  // Threshold: 50ms
}

// Performance test for sensor_get_distance (should complete <10ms)
TEST_CASE("Sensor Distance Performance", "[performance]") {
    // Setup config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    sensor_init();

    // Measure time
    auto start = std::chrono::high_resolution_clock::now();
    float dist = sensor_get_distance();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    REQUIRE(duration.count() < 10);  // Threshold: 10ms
}

// Performance test for config loading (should complete <100ms)
TEST_CASE("Config Load Performance", "[performance]") {
    // Mock LittleFS (assume loadConfig works)
    auto start = std::chrono::high_resolution_clock::now();
    bool result = loadConfig();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    REQUIRE(duration.count() < 100);  // Threshold: 100ms
    REQUIRE(result == true);
}
