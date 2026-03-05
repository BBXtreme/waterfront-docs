// test_config_loader.cpp - Unit tests for config_loader.cpp using Catch2
// This file contains tests for config loader functions, focusing on JSON parsing, validation, and file operations.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_RUNNER  // Catch2 runner for multiple test files
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "config_loader.h"

// Define global config for tests
GlobalConfig g_config;

// Mock LittleFS for file operations
class MockLittleFS {
public:
    static bool begin() { return mockBegin; }
    static FILE* open(const char* path, const char* mode) {
        if (strcmp(path, "/littlefs/config.json") == 0) {
            return mockFile;
        }
        return nullptr;
    }
    static bool remove(const char* path) { return mockRemove; }
    static std::string mockContent;
    static bool mockBegin;
    static bool mockRemove;
    static FILE* mockFile;
};
std::string MockLittleFS::mockContent = "";
bool MockLittleFS::mockBegin = true;
bool MockLittleFS::mockRemove = true;
FILE* MockLittleFS::mockFile = nullptr;

// Mock FILE functions
size_t mockFread(void* ptr, size_t size, size_t count, FILE* stream) {
    if (stream == MockLittleFS::mockFile) {
        memcpy(ptr, MockLittleFS::mockContent.c_str(), MockLittleFS::mockContent.size());
        return MockLittleFS::mockContent.size();
    }
    return 0;
}
int mockFseek(FILE* stream, long offset, int whence) { return 0; }
long mockFtell(FILE* stream) { return MockLittleFS::mockContent.size(); }
int mockFclose(FILE* stream) { return 0; }
FILE* mockFopen(const char* path, const char* mode) { return MockLittleFS::mockFile; }
size_t mockFwrite(const void* ptr, size_t size, size_t count, FILE* stream) { return count; }
int mockUnlink(const char* path) { return 0; }
#define fread mockFread
#define fseek mockFseek
#define ftell mockFtell
#define fclose mockFclose
#define fopen mockFopen
#define fwrite mockFwrite
#define unlink mockUnlink

// Override LittleFS
#define LittleFS MockLittleFS

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
    REQUIRE(g_config.mqtt.broker == "8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud");  // Default
}

// Test load config with missing file
TEST_CASE("Load Config - Missing File", "[config]") {
    // Mock file not found
    MockLittleFS::mockFile = nullptr;

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
    REQUIRE(def.mqtt.broker == "8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud");
    REQUIRE(def.location.slug == "bremen");
    REQUIRE(def.compartmentCount == 1);
}

// New: Test config save failure
TEST_CASE("Save Config - File Write Failure", "[config]") {
    // Mock file open failure
    MockLittleFS::mockFile = nullptr;

    // Call save
    bool result = saveConfig();

    // Verify failure
    REQUIRE(result == false);
}

// New: Test config update with partial JSON
TEST_CASE("Update Config - Partial JSON", "[config]") {
    // Mock partial JSON (only MQTT)
    const char* json = "{\"mqtt\":{\"broker\":\"partial.com\"}}";

    // Call update
    bool result = updateConfigFromJson(json);

    // Verify success and partial update
    REQUIRE(result == true);
    REQUIRE(g_config.mqtt.broker == "partial.com");
    // Other fields should remain default or previous
}
