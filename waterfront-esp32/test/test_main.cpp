// test_main.cpp - Unit tests for main.cpp utility functions using Catch2
// This file contains tests for battery, solar, and deep sleep functions.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks ADC and ESP functions to simulate hardware without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include functions from main.cpp (copy or extern)
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Mock analogRead
int mockAnalogReadValues[40] = {2048};  // Default 2048 for 50%
int mockAnalogRead(int pin) {
    return mockAnalogReadValues[pin];
}
#define analogRead mockAnalogRead

// Mock ESP functions
#define ESP_LOGI(tag, fmt, ...) // Mock log
#define ESP_LOGE(tag, fmt, ...) // Mock log

// Mock esp_deep_sleep_start
bool deepSleepCalled = false;
#define esp_deep_sleep_start() deepSleepCalled = true

// Mock esp_sleep_enable_timer_wakeup
void mockEspSleepEnableTimerWakeup(uint64_t time) {}
#define esp_sleep_enable_timer_wakeup mockEspSleepEnableTimerWakeup

// Mock esp_sleep_enable_ext0_wakeup
void mockEspSleepEnableExt0Wakeup(int gpio, int level) {}
#define esp_sleep_enable_ext0_wakeup mockEspSleepEnableExt0Wakeup

// Copy functions from main.cpp
int readBatteryLevel() {
    int adcValue = analogRead(34);  // ADC pin for battery
    if (adcValue < 0 || adcValue > 4095) {
        ESP_LOGE("MAIN", "Invalid ADC value for battery: %d", adcValue);
        return 0;  // Fallback
    }
    // Map ADC (0-4095) to percentage (0-100), calibrate as needed
    int batteryPercent = map(adcValue, 0, 4095, 0, 100);
    return batteryPercent;
}

float readSolarVoltage() {
    int adcValue = analogRead(35);  // ADC pin for solar
    if (adcValue < 0 || adcValue > 4095) {
        ESP_LOGE("MAIN", "Invalid ADC value for solar: %f", adcValue);
        return 0.0f;  // Fallback
    }
    // Calculate voltage: ADC * (3.3V / 4095) * divider ratio
    float voltage = (adcValue / 4095.0) * 3.3 * (10.0 / 3.3);
    return voltage;
}

void enterDeepSleep() {
    ESP_LOGI("MAIN", "Entering deep sleep due to low power conditions");
    // Enable timer wakeup for 60 seconds
    esp_sleep_enable_timer_wakeup(60 * 1000000);
    // Enable GPIO wakeup on pin 0 low
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
    esp_deep_sleep_start();  // Enter deep sleep
}

// Test readBatteryLevel
TEST_CASE("Read Battery Level", "[main]") {
    // Set mock ADC
    mockAnalogReadValues[34] = 2048;  // 50%

    // Call read
    int level = readBatteryLevel();

    // Verify
    REQUIRE(level == 50);
}

// Test readBatteryLevel - invalid
TEST_CASE("Read Battery Level - Invalid", "[main]") {
    // Set mock ADC invalid
    mockAnalogReadValues[34] = 5000;

    // Call read
    int level = readBatteryLevel();

    // Verify fallback
    REQUIRE(level == 0);
}

// Test readSolarVoltage
TEST_CASE("Read Solar Voltage", "[main]") {
    // Set mock ADC
    mockAnalogReadValues[35] = 2048;

    // Call read
    float voltage = readSolarVoltage();

    // Verify approx
    REQUIRE(voltage == Approx(5.0f).epsilon(0.1));
}

// Test readSolarVoltage - invalid
TEST_CASE("Read Solar Voltage - Invalid", "[main]") {
    // Set mock ADC invalid
    mockAnalogReadValues[35] = 5000;

    // Call read
    float voltage = readSolarVoltage();

    // Verify fallback
    REQUIRE(voltage == 0.0f);
}

// Test enterDeepSleep
TEST_CASE("Enter Deep Sleep", "[main]") {
    // Call enter
    enterDeepSleep();

    // Verify
    REQUIRE(deepSleepCalled == true);
}
