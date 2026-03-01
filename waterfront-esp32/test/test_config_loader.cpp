// test_config_loader.cpp - Unit tests for config_loader.cpp using Catch2
// This file contains tests for config loader functions, focusing on JSON parsing, validation, and file operations.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks LittleFS and ArduinoJson to simulate file system without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_RUNNER  // Catch2 runner for multiple test files
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "config_loader.h"

// Mock LittleFS for file operations
class MockLittleFS {
public:
    static bool begin() { return mockBegin; }
    static File open(const char* path, const char* mode) {
        if (strcmp(path, "/config.json") == 0) {
            return MockFile(mockContent, mode);
        }
        return MockFile("", mode);
    }
    static bool remove(const char* path) { return mockRemove; }
    static String mockContent;
    static bool mockBegin;
    static bool mockRemove;
};
String MockLittleFS::mockContent = "";
bool MockLittleFS::mockBegin = true;
bool MockLittleFS::mockRemove = true;

// Override LittleFS
#define LittleFS MockLittleFS

// Mock File class
class MockFile {
public:
    MockFile(const String& content, const char* mode) : content(content), position(0), size(content.length()) {}
    size_t size() { return size; }
    void print(const char* str) { written += str; }
    void close() {}
    String content;
    size_t position;
    size_t size;
    String written;
};

// Override File
#define File MockFile

// Test load config with valid JSON
TEST_CASE("Load Config - Valid JSON", "[config]") {
    // Set mock content
    MockLittleFS::mockContent = "{\"mqtt\":{\"broker\":\"test.com\",\"port\":1883,\"useTLS\":false},\"location\":{\"slug\":\"test\",\"code\":\"test-01\"},\"compartments\":[{\"number\":1,\"servoPin\":12,\"limitOpenPin\":13,\"limitClosePin\":14,\"ultrasonicTriggerPin\":15,\"ultrasonicEchoPin\":16,\"weightSensorPin\":17}],\"system\":{\"debugMode\":true,\"gracePeriodSec\":3600,\"batteryLowThresholdPercent\":20,\"solarVoltageMin\":3.0}}";

    // Call load
    bool result = loadConfig();

    // Verify success
    REQUIRE(result == true);
    REQUIRE(g_config.mqtt.broker == "test.com");
    REQUIRE(g_config.location.slug == "test");
    REQUIRE(g_config.compartmentCount == 1);
}

// Test load config with invalid JSON
TEST_CASE("Load Config - Invalid JSON", "[config]") {
    // Set mock invalid content
    MockLittleFS::mockContent = "{\"invalid\":json}";

    // Call load
    bool result = loadConfig();

    // Verify fallback to defaults
    REQUIRE(result == false);
    REQUIRE(g_config.mqtt.broker == "192.168.178.50");  // Default
}

// Test load config with missing file
TEST_CASE("Load Config - Missing File", "[config]") {
    // Mock file not found
    MockLittleFS::mockContent = "";  // Empty means not found

    // Call load
    bool result = loadConfig();

    // Verify fallback
    REQUIRE(result == false);
}

// Test load config with empty file
TEST_CASE("Load Config - Empty File", "[config]") {
    // Set mock empty content
    MockLittleFS::mockContent = "";

    // Call load
    bool result = loadConfig();

    // Verify fallback
    REQUIRE(result == false);
}

// Test load config with LittleFS failure
TEST_CASE("Load Config - LittleFS Failure", "[config]") {
    // Mock LittleFS begin failure
    MockLittleFS::mockBegin = false;

    // Call load
    bool result = loadConfig();

    // Verify failure
    REQUIRE(result == false);

    // Reset mock
    MockLittleFS::mockBegin = true;
}

// Test validate config - valid
TEST_CASE("Validate Config - Valid", "[config]") {
    // Set valid config
    g_config.mqtt.port = 1883;
    g_config.lte.rssiThreshold = -70;
    g_config.system.maxCompartments = 10;
    g_config.system.gracePeriodSec = 3600;
    g_config.system.batteryLowThresholdPercent = 20;
    g_config.system.solarVoltageMin = 3.0f;
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Call validate
    bool result = validateConfig(g_config);

    // Verify true
    REQUIRE(result == true);
}

// Test validate config - invalid port
TEST_CASE("Validate Config - Invalid Port", "[config]") {
    // Set invalid port
    g_config.mqtt.port = 0;

    // Call validate
    bool result = validateConfig(g_config);

    // Verify false
    REQUIRE(result == false);
}

// Test validate config - invalid pin
TEST_CASE("Validate Config - Invalid Pin", "[config]") {
    // Set valid base
    g_config.mqtt.port = 1883;
    g_config.lte.rssiThreshold = -70;
    g_config.system.maxCompartments = 10;
    g_config.system.gracePeriodSec = 3600;
    g_config.system.batteryLowThresholdPercent = 20;
    g_config.system.solarVoltageMin = 3.0f;
    g_config.compartments[0] = {1, 50, 13, 14, 15, 16, 17};  // Invalid servo pin
    g_config.compartmentCount = 1;

    // Call validate
    bool result = validateConfig(g_config);

    // Verify false
    REQUIRE(result == false);
}

// Test update config from JSON
TEST_CASE("Update Config from JSON", "[config]") {
    // Mock valid JSON
    const char* json = "{\"mqtt\":{\"broker\":\"updated.com\"},\"system\":{\"debugMode\":false}}";

    // Call update
    bool result = updateConfigFromJson(json);

    // Verify success
    REQUIRE(result == true);
    REQUIRE(g_config.mqtt.broker == "updated.com");
    REQUIRE(g_config.system.debugMode == false);
}

// Test update config from invalid JSON
TEST_CASE("Update Config from Invalid JSON", "[config]") {
    // Mock invalid JSON
    const char* json = "{\"invalid\":json}";

    // Call update
    bool result = updateConfigFromJson(json);

    // Verify failure
    REQUIRE(result == false);
}

// Test update config with empty payload
TEST_CASE("Update Config - Empty Payload", "[config]") {
    // Call update with empty
    bool result = updateConfigFromJson("");

    // Verify failure
    REQUIRE(result == false);
}

// Test get default config
TEST_CASE("Get Default Config", "[config]") {
    // Call get default
    GlobalConfig def = getDefaultConfig();

    // Verify defaults
    REQUIRE(def.mqtt.broker == "192.168.178.50");
    REQUIRE(def.location.slug == "bremen");
    REQUIRE(def.compartmentCount == 1);
}
