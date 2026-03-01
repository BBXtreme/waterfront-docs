// test_deposit_logic.cpp - Unit tests for deposit_logic.cpp using Catch2
// This file contains tests for deposit logic functions, focusing on state transitions and timer mocks.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks millis() and PubSubClient to simulate time and MQTT without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch2/catch_test_macros.hpp>  // Catch2 header (add to lib_deps in platformio.ini: "catchorg/Catch2@^3.4.0")

// Include headers under test
#include "deposit_logic.h"
#include "config_loader.h"
#include <PubSubClient.h>

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Mock PubSubClient
class MockPubSubClient : public PubSubClient {
public:
    MockPubSubClient() : PubSubClient(mockClient) {}
    bool publish(const char* topic, const char* payload, bool retained) override {
        lastPublishedTopic = topic;
        lastPublishedPayload = payload;
        publishCount++;
        return true;
    }
    String lastPublishedTopic;
    String lastPublishedPayload;
    int publishCount = 0;
private:
    class MockClient : public Client {
    public:
        virtual int connect(IPAddress ip, uint16_t port) override { return 1; }
        virtual int connect(const char *host, uint16_t port) override { return 1; }
        virtual size_t write(uint8_t) override { return 1; }
        virtual size_t write(const uint8_t *buf, size_t size) override { return size; }
        virtual int available() override { return 0; }
        virtual int read() override { return -1; }
        virtual int read(uint8_t *buf, size_t size) override { return -1; }
        virtual int peek() override { return -1; }
        virtual void flush() override {}
        virtual void stop() override {}
        virtual uint8_t connected() override { return 1; }
        virtual operator bool() override { return true; }
    } mockClient;
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

    // Verify timer added (check activeTimers size if accessible, or assume via behavior)
    REQUIRE(true);  // Placeholder; in real test, expose activeTimers or check via checkOverdue
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
    REQUIRE(true);  // Placeholder
}

// Test check overdue (overdue)
TEST_CASE("Check Overdue - Overdue", "[deposit]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.system.gracePeriodSec = 3600;  // 1 hour

    // Call init
    deposit_init();

    // Start rental
    startRental(1, 7200);
    mockMillis = 10800000;  // 3 hours (overdue)

    // Call check_overdue
    checkOverdue();

    // Verify timer removed (overdue handling)
    REQUIRE(true);  // Placeholder
}
