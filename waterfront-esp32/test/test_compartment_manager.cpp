// test_compartment_manager.cpp - Unit tests for compartment_manager.cpp using Catch2
// This file contains tests for compartment manager functions, focusing on loading from mock JSON and pin assignment.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks LittleFS and ArduinoJson to simulate config loading without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch2/catch_test_macros.hpp>  // Catch2 header (add to lib_deps in platformio.ini: "catchorg/Catch2@^3.4.0")

// Include headers under test
#include "compartment_manager.h"
#include "config_loader.h"
#include <ArduinoJson.h>

// Mock GlobalConfig for tests
GlobalConfig g_config;
void loadConfig() { // Mock load
    g_config.compartments.push_back({1, 12, 13, 14, 15, 16, 17});
    g_config.compartments.push_back({2, 15, 16, 17, 18, 19, 20});
}

// Mock LittleFS for config loading
class MockLittleFS {
public:
    static bool begin() { return true; }
    static File open(const char* path, const char* mode) {
        if (strcmp(path, "/config.json") == 0 && strcmp(mode, "r") == 0) {
            return MockFile(mockConfigJson);
        }
        return MockFile("");
    }
    static const char* mockConfigJson;
};

const char* MockLittleFS::mockConfigJson = R"(
{
  "mqtt": {"broker": "test.broker", "port": 1883},
  "location": {"slug": "test", "code": "test-01"},
  "compartments": [
    {"number": 1, "servoPin": 12, "limitOpenPin": 13, "limitClosePin": 14},
    {"number": 2, "servoPin": 15, "limitOpenPin": 16, "limitClosePin": 17}
  ]
}
)";

// Mock File class
class MockFile {
public:
    MockFile(const char* content) : content(content), position(0) {}
    size_t readString() { return content; }
    operator bool() { return content != nullptr && strlen(content) > 0; }
private:
    const char* content;
    size_t position;
};

// Override LittleFS for tests
#define LittleFS MockLittleFS

// Test load compartments from mock JSON
TEST_CASE("Load Compartments from Config", "[compartment]") {
    // Call load (assuming it uses LittleFS)
    load_compartments();

    // Get compartments
    auto comps = get_all_compartments();

    // Verify loaded 2 compartments
    REQUIRE(comps.size() == 2);

    // Verify first compartment pins
    REQUIRE(comps[0].id == 1);
    REQUIRE(comps[0].servoPin == 12);
    REQUIRE(comps[0].limitOpenPin == 13);
    REQUIRE(comps[0].limitClosePin == 14);

    // Verify second compartment pins
    REQUIRE(comps[1].id == 2);
    REQUIRE(comps[1].servoPin == 15);
    REQUIRE(comps[1].limitOpenPin == 16);
    REQUIRE(comps[1].limitClosePin == 17);
}

// Test get compartment by ID
TEST_CASE("Get Compartment by ID", "[compartment]") {
    // Assuming load_compartments called in previous test
    Compartment* comp = get_compartment(1);

    // Verify returns correct compartment
    REQUIRE(comp != nullptr);
    REQUIRE(comp->id == 1);
    REQUIRE(comp->servoPin == 12);
}

// Test get compartment by invalid ID
TEST_CASE("Get Compartment by Invalid ID", "[compartment]") {
    Compartment* comp = get_compartment(99);

    // Verify returns nullptr
    REQUIRE(comp == nullptr);
}

// Test compartment name generation
TEST_CASE("Compartment Name Generation", "[compartment]") {
    // Load compartments
    load_compartments();

    // Get compartments
    auto comps = get_all_compartments();

    // Verify names
    REQUIRE(std::string(comps[0].name) == "Compartment 1");
    REQUIRE(std::string(comps[1].name) == "Compartment 2");
}

// Test empty config fallback
TEST_CASE("Empty Config Fallback", "[compartment]") {
    // Clear compartments
    g_config.compartments.clear();

    // Call load
    load_compartments();

    // Get compartments
    auto comps = get_all_compartments();

    // Verify fallback to default
    REQUIRE(comps.size() == 1);
    REQUIRE(comps[0].id == 1);
    REQUIRE(std::string(comps[0].name) == "Compartment 1");
}
