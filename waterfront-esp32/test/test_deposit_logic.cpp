// test_deposit_logic.cpp - Unit tests for deposit_logic.cpp using Catch2
// This file contains tests for deposit logic functions, focusing on state transitions and timer mocks.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks millis() and PubSubClient to simulate time and MQTT without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_RUNNER  // Catch2 runner for multiple test files
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "deposit_logic.h"
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Minimal PubSubClient base class for mocking
class PubSubClient {
public:
    virtual bool publish(const char* topic, const char* payload, bool retained) = 0;
};

// Mock PubSubClient
class MockPubSubClient : public PubSubClient {
public:
    MockPubSubClient() : PubSubClient() {}
    bool publish(const char* topic, const char* payload, bool retained) override {
        lastPublishedTopic = topic;
        lastPublishedPayload = payload;
        publishCount++;
        return true;
    }
    String lastPublishedTopic;
    String lastPublishedPayload;
    int publishCount = 0;
};

// Global mock instance
MockPubSubClient mockMqttClient;

// Override extern mqttClient for tests
PubSubClient& mqttClient = mockMqttClient;

// Mock millis for timer tests
unsigned long mockMillis = 0;
#define millis() mockMillis

// Test deposit initialization
TEST_CASE("Deposit Initialization", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Verify deposit not held initially
    REQUIRE(deposit_is_held() == false);
}

// Test deposit on take
TEST_CASE("Deposit On Take", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Call on_take
    deposit_on_take();

    // Verify deposit held
    REQUIRE(deposit_is_held() == true);
}

// Test deposit on return (on time)
TEST_CASE("Deposit On Return - On Time", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Simulate take
    deposit_on_take();
    REQUIRE(deposit_is_held() == true);

    // Simulate time passing (within grace period)
    mockMillis = 1800000;  // 30 minutes

    // Call on_return
    deposit_on_return(&mqttClient);

    // Verify deposit released
    REQUIRE(deposit_is_held() == false);
}

// Test deposit on return (overdue)
TEST_CASE("Deposit On Return - Overdue", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Simulate take
    deposit_on_take();
    REQUIRE(deposit_is_held() == true);

    // Simulate time passing (over grace period)
    mockMillis = 7200000;  // 2 hours

    // Call on_return
    deposit_on_return(&mqttClient);

    // Verify deposit still held (overdue)
    REQUIRE(deposit_is_held() == true);
}

// Test start rental timer
TEST_CASE("Start Rental Timer", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Start rental for 2 hours
    startRental(1, 7200);

    // Verify timer added
    REQUIRE(activeTimers.size() == 1);
    REQUIRE(activeTimers[0].compartmentId == 1);
    REQUIRE(activeTimers[0].durationSec == 7200);
}

// Test start rental timer with multiple compartments
TEST_CASE("Start Rental Timer - Multiple", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartments[1] = {2, 18, 19, 20, 21, 22, 23};
    g_config.compartmentCount = 2;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Start rentals for both compartments
    startRental(1, 7200);
    startRental(2, 3600);

    // Verify timers added
    REQUIRE(activeTimers.size() == 2);
    REQUIRE(activeTimers[0].compartmentId == 1);
    REQUIRE(activeTimers[0].durationSec == 7200);
    REQUIRE(activeTimers[1].compartmentId == 2);
    REQUIRE(activeTimers[1].durationSec == 3600);
}

// Test check overdue (no overdue)
TEST_CASE("Check Overdue - No Overdue", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Start rental
    startRental(1, 7200);
    mockMillis = 3600000;  // 1 hour

    // Call check_overdue
    checkOverdue();

    // Verify no action (timers still active)
    REQUIRE(activeTimers.size() == 1);
}

// Test check overdue (overdue)
TEST_CASE("Check Overdue - Overdue", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour
    g_config.location.code = "harbor-01";

    // Call init
    deposit_init();

    // Start rental
    startRental(1, 7200);
    mockMillis = 10800000;  // 3 hours (overdue)

    // Call check_overdue
    checkOverdue();

    // Verify auto-lock publish
    REQUIRE(mockMqttClient.publishCount == 1);
    REQUIRE(mockMqttClient.lastPublishedTopic == "waterfront/locations/harbor-01/returnConfirm");
    REQUIRE(mockMqttClient.lastPublishedPayload.contains("\"action\":\"autoLock\""));
    // Verify timer removed
    REQUIRE(activeTimers.size() == 0);
}

// Test check overdue with multiple timers
TEST_CASE("Check Overdue - Multiple Timers", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartments[1] = {2, 18, 19, 20, 21, 22, 23};
    g_config.compartmentCount = 2;
    g_config.system.gracePeriodSec = 3600;  // 1 hour
    g_config.location.code = "harbor-01";

    // Call init
    deposit_init();

    // Start rentals
    startRental(1, 7200);
    startRental(2, 3600);
    mockMillis = 10800000;  // 3 hours (both overdue)

    // Call check_overdue
    checkOverdue();

    // Verify publishes for both
    REQUIRE(mockMqttClient.publishCount == 2);
    // Verify timers removed
    REQUIRE(activeTimers.size() == 0);
}

// Test deposit on return without prior take
TEST_CASE("Deposit On Return - No Prior Take", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Call on_return without take
    deposit_on_return(&mqttClient);

    // Verify no change
    REQUIRE(deposit_is_held() == false);
}
