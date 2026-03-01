// test_mqtt_client.cpp - Unit tests for mqtt_client.cpp using Catch2
// This file contains tests for MQTT client initialization, publishing, and loop.
// Uses Catch2 for test framework (header-only, include via PlatformIO lib_deps).
// Mocks PubSubClient, WiFiClient, LittleFS to simulate MQTT without network.
// Run tests with PlatformIO: pio test

#define CATCH_CONFIG_MAIN  // Catch2 main entry point
#include <catch.hpp>  // Catch2 header (add to lib_deps in platformio.ini)

// Include headers under test
#include "mqtt_client.h"
#include "config_loader.h"

// Mock GlobalConfig for tests
GlobalConfig g_config;

// Mock WiFiClient
class MockWiFiClient : public WiFiClient {
public:
    int connect(IPAddress ip, uint16_t port) override { return 1; }
    size_t write(uint8_t b) override { return 1; }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    uint8_t connected() override { return 1; }
    operator bool() override { return true; }
};
MockWiFiClient mockEspClient;
#define espClient mockEspClient

// Mock PubSubClient
class MockPubSubClient : public PubSubClient {
public:
    MockPubSubClient(Client& client) : PubSubClient(client) {}
    bool connect(const char* id) override { return mockConnected; }
    bool connect(const char* id, const char* user, const char* pass) override { return mockConnected; }
    bool publish(const char* topic, const char* payload, bool retained) override { publishCount++; return true; }
    void setServer(const char* server, int port) override { serverSet = true; }
    void setCallback(MQTT_CALLBACK_SIGNATURE) override { callbackSet = true; }
    bool connected() override { return mockConnected; }
    void loop() override {}
    int state() override { return 0; }
    static bool mockConnected;
    static bool serverSet;
    static bool callbackSet;
    static int publishCount;
};
bool MockPubSubClient::mockConnected = false;
bool MockPubSubClient::serverSet = false;
bool MockPubSubClient::callbackSet = false;
int MockPubSubClient::publishCount = 0;
MockPubSubClient mockMqttClient(mockEspClient);
#define mqttClient mockMqttClient

// Mock LittleFS
class MockLittleFS {
public:
    static bool begin() { return true; }
    static File open(const char* path, const char* mode) { return MockFile(); }
};
#define LittleFS MockLittleFS

// Mock File
class MockFile {
public:
    size_t size() { return 0; }
    String readString() { return ""; }
    operator bool() { return false; }
    void close() {}
};

// Mock ESP_LOG
#define ESP_LOGI(tag, fmt, ...) // Mock log
#define ESP_LOGE(tag, fmt, ...) // Mock log
#define ESP_LOGW(tag, fmt, ...) // Mock log

// Mock millis
unsigned long mockMillis = 0;
#define millis() mockMillis

// Test mqtt_init
TEST_CASE("MQTT Client Init", "[mqtt_client]") {
    // Set mock config
    g_config.mqtt.broker = "test.com";
    g_config.mqtt.port = 1883;
    MockPubSubClient::mockConnected = true;

    // Call init
    esp_err_t result = mqtt_init();

    // Verify
    REQUIRE(result == ESP_OK);
    REQUIRE(MockPubSubClient::serverSet == true);
    REQUIRE(MockPubSubClient::callbackSet == true);
}

// Test mqtt_publish_status
TEST_CASE("MQTT Publish Status", "[mqtt_client]") {
    // Set mock config
    g_config.location.code = "test-01";
    MockPubSubClient::mockConnected = true;

    // Call publish
    mqtt_publish_status();

    // Verify
    REQUIRE(MockPubSubClient::publishCount == 1);
}

// Test mqtt_publish_slot_status
TEST_CASE("MQTT Publish Slot Status", "[mqtt_client]") {
    // Set mock config
    MockPubSubClient::mockConnected = true;

    // Call publish
    mqtt_publish_slot_status(1, "{\"status\":\"open\"}");

    // Verify
    REQUIRE(MockPubSubClient::publishCount == 1);
}

// Test getMqttReconnectCount
TEST_CASE("Get MQTT Reconnect Count", "[mqtt_client]") {
    // Call get
    int count = getMqttReconnectCount();

    // Verify (starts at 0)
    REQUIRE(count == 0);
}
