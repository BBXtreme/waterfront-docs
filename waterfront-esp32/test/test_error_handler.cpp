// test_error_handler.cpp - Unit tests for error_handler.cpp using Catch2
// This file contains tests for error handling functions, focusing on fatal errors and MQTT alerting.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks PubSubClient and ESP restart to simulate error handling without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_RUNNER  // Catch2 runner for multiple test files
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "error_handler.h"
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Minimal PubSubClient base class for mocking
class PubSubClient {
public:
    virtual bool publish(const char* topic, const char* payload, bool retained) = 0;
    virtual bool connected() = 0;
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
    bool connected() override { return mockConnected; }
    String lastPublishedTopic;
    String lastPublishedPayload;
    int publishCount = 0;
    static bool mockConnected;
};
bool MockPubSubClient::mockConnected = false;

// Global mock instance
MockPubSubClient mockMqttClient;

// Override extern mqttClient for tests
PubSubClient& mqttClient = mockMqttClient;

// Mock ESP restart
bool espRestartCalled = false;
#define esp_restart() espRestartCalled = true

// Mock ESP log
#define ESP_LOGE(tag, fmt, ...) // Mock log
#define ESP_LOGW(tag, fmt, ...) // Mock log
#define ESP_LOGI(tag, fmt, ...) // Mock log

// Test fatal error with MQTT connected and debug mode
TEST_CASE("Fatal Error - MQTT Connected Debug Mode", "[error]") {
    // Set mock config
    g_config.debugMode = true;
    g_config.location.slug = "bremen";
    g_config.location.code = "harbor-01";
    MockPubSubClient::mockConnected = true;
    espRestartCalled = false;
    mockMqttClient.publishCount = 0;

    // Call fatal_error
    fatal_error("Test error", ESP_FAIL);

    // Verify debug publish
    REQUIRE(mockMqttClient.publishCount == 2);  // Debug and alert
    REQUIRE(mockMqttClient.lastPublishedTopic == "waterfront/bremen/harbor-01/alert");
    REQUIRE(mockMqttClient.lastPublishedPayload.contains("\"error\":\"Test error\""));
    REQUIRE(espRestartCalled == true);
}

// Test fatal error with MQTT disconnected
TEST_CASE("Fatal Error - MQTT Disconnected", "[error]") {
    // Set mock config
    g_config.debugMode = true;
    MockPubSubClient::mockConnected = false;
    espRestartCalled = false;
    mockMqttClient.publishCount = 0;

    // Call fatal_error
    fatal_error("Test error", ESP_FAIL);

    // Verify only alert publish (if connected, but mocked as disconnected)
    REQUIRE(mockMqttClient.publishCount == 1);  // Alert only
    REQUIRE(espRestartCalled == true);
}

// Test fatal error with debug mode off
TEST_CASE("Fatal Error - Debug Mode Off", "[error]") {
    // Set mock config
    g_config.debugMode = false;
    MockPubSubClient::mockConnected = true;
    espRestartCalled = false;
    mockMqttClient.publishCount = 0;

    // Call fatal_error
    fatal_error("Test error", ESP_FAIL);

    // Verify only alert publish
    REQUIRE(mockMqttClient.publishCount == 1);
    REQUIRE(mockMqttClient.lastPublishedTopic == "waterfront/bremen/harbor-01/alert");
    REQUIRE(espRestartCalled == true);
}

// Test fatal error with null message
TEST_CASE("Fatal Error - Null Message", "[error]") {
    // Set mock config
    g_config.debugMode = true;
    MockPubSubClient::mockConnected = true;
    espRestartCalled = false;
    mockMqttClient.publishCount = 0;

    // Call fatal_error with null
    fatal_error(NULL, ESP_FAIL);

    // Verify uses fallback message
    REQUIRE(mockMqttClient.publishCount == 2);
    REQUIRE(mockMqttClient.lastPublishedPayload.contains("\"error\":\"Unknown error\""));
    REQUIRE(espRestartCalled == true);
}

// Test fatal error with different error codes
TEST_CASE("Fatal Error - Different Error Codes", "[error]") {
    // Set mock config
    g_config.debugMode = true;
    MockPubSubClient::mockConnected = true;
    espRestartCalled = false;
    mockMqttClient.publishCount = 0;

    // Call fatal_error with different code
    fatal_error("Test error", ESP_ERR_INVALID_ARG);

    // Verify code is logged
    REQUIRE(mockMqttClient.publishCount == 2);
    REQUIRE(espRestartCalled == true);
}
