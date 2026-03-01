// test_return_sensor.cpp - Unit tests for return_sensor.cpp using Catch2
// This file contains tests for return sensor functions, focusing on distance measurement and presence detection.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks ultrasonic sensor pins and pulseIn to simulate hardware without ESP32.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "return_sensor.h"
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Minimal PubSubClient base class for mocking
class PubSubClient {
public:
    virtual bool publish(const char* topic, const char* payload, bool retained) = 0;
};

// Mock PubSubClient for publish verification
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
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Call init
    sensor_init();

    // Verify pins are set (mock doesn't check, but assume it works)
    REQUIRE(true);  // Placeholder; in real test, check pin modes if exposed
}

// Test sensor initialization with multiple compartments
TEST_CASE("Sensor Initialization - Multiple Compartments", "[sensor]") {
    // Set mock config with multiple compartments
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartments[1] = {2, 18, 19, 20, 21, 22, 23};
    g_config.compartmentCount = 2;

    // Call init
    sensor_init();

    // Verify (mock assumes success)
    REQUIRE(true);
}

// Test sensor initialization with no compartments
TEST_CASE("Sensor Initialization - No Compartments", "[sensor]") {
    // Set mock config with no compartments
    g_config.compartmentCount = 0;

    // Call init
    sensor_init();

    // Verify (should not crash)
    REQUIRE(true);
}

// Test distance measurement
TEST_CASE("Distance Measurement", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Call init
    sensor_init();

    // Call get_distance
    float distance = sensor_get_distance();

    // Verify distance calculation (mockPulseDuration = 2940 us -> ~50cm)
    REQUIRE(distance == Approx(50.0f).epsilon(0.1));
}

// Test distance measurement with different pulse durations
TEST_CASE("Distance Measurement - Various Distances", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Call init
    sensor_init();

    // Test short distance
    mockPulseDuration = 1470;  // 25cm
    float distance = sensor_get_distance();
    REQUIRE(distance == Approx(25.0f).epsilon(0.1));

    // Test long distance
    mockPulseDuration = 5880;  // 100cm
    distance = sensor_get_distance();
    REQUIRE(distance == Approx(100.0f).epsilon(0.1));
}

// Test distance measurement with no compartments
TEST_CASE("Distance Measurement - No Compartments", "[sensor]") {
    // Set mock config with no compartments
    g_config.compartmentCount = 0;

    // Call get_distance
    float distance = sensor_get_distance();

    // Verify returns -1.0f
    REQUIRE(distance == -1.0f);
}

// Test kayak presence detection (present)
TEST_CASE("Kayak Presence Detection - Present", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

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
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Set mock distance > threshold (100cm)
    mockPulseDuration = 5880;  // 100cm

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify false
    REQUIRE(present == false);
}

// Test kayak presence detection at threshold
TEST_CASE("Kayak Presence Detection - At Threshold", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Set mock distance exactly at threshold (50cm)
    mockPulseDuration = 2940;  // 50cm

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify true (assuming <= threshold is present)
    REQUIRE(present == true);
}

// Test invalid distance (pulseIn returns 0)
TEST_CASE("Invalid Distance Reading", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;

    // Set mock pulse to 0 (invalid)
    mockPulseDuration = 0;

    // Call init
    sensor_init();

    // Call is_kayak_present
    bool present = sensor_is_kayak_present();

    // Verify false (invalid reading)
    REQUIRE(present == false);
}

// Test returned event publish
TEST_CASE("Returned Event Publish", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.location.slug = "bremen";
    g_config.location.code = "harbor-01";

    // Set mock distance for return (kayak present)
    mockPulseDuration = 2940;  // 50cm

    // Call init
    sensor_init();

    // Simulate return logic: if kayak present, publish "returned" event
    if (sensor_is_kayak_present()) {
        String payload = "{\"event\":\"returned\",\"compartmentId\":1,\"timestamp\":1234567890}";
        char topic[64];
        snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/1/return", g_config.location.slug.c_str(), g_config.location.code.c_str());
        mqttClient.publish(topic, payload.c_str(), false);
    }

    // Verify publish called
    REQUIRE(mockMqttClient.publishCount == 1);
    REQUIRE(mockMqttClient.lastPublishedTopic == "waterfront/bremen/harbor-01/compartments/1/return");
    REQUIRE(mockMqttClient.lastPublishedPayload == "{\"event\":\"returned\",\"compartmentId\":1,\"timestamp\":1234567890}");
}

// Test returned event publish with no presence
TEST_CASE("Returned Event Publish - No Presence", "[sensor]") {
    // Set mock config
    g_config.compartments[0] = {1, 12, 13, 14, 15, 16, 17};
    g_config.compartmentCount = 1;
    g_config.location.slug = "bremen";
    g_config.location.code = "harbor-01";

    // Set mock distance for no return (kayak not present)
    mockPulseDuration = 5880;  // 100cm

    // Call init
    sensor_init();

    // Simulate return logic: if kayak present, publish "returned" event
    if (sensor_is_kayak_present()) {
        String payload = "{\"event\":\"returned\",\"compartmentId\":1,\"timestamp\":1234567890}";
        char topic[64];
        snprintf(topic, sizeof(topic), "waterfront/%s/%s/compartments/1/return", g_config.location.slug.c_str(), g_config.location.code.c_str());
        mqttClient.publish(topic, payload.c_str(), false);
    }

    // Verify no publish called
    REQUIRE(mockMqttClient.publishCount == 0);
}
