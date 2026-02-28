// nimble.cpp - BLE (Bluetooth Low Energy) provisioning implementation
// This file handles BLE-based WiFi provisioning for the ESP32.
// It uses the Arduino BLE library (NimBLE) to advertise a GATT service for SSID/password input.
// When credentials are received, it connects to WiFi and publishes status via MQTT.
// This is the preferred provisioning method (secure, works with apps like nRF Connect).

#include "nimble.h"           // Header for BLE functions
#include "config.h"           // Configuration constants
#include <BLEDevice.h>        // BLE device management
#include <BLEServer.h>        // BLE server for GATT services
#include <BLEUtils.h>         // BLE utilities
#include <BLE2902.h>          // Descriptor for notifications
#include <WiFi.h>             // WiFi library for connection
#include <PubSubClient.h>     // MQTT client
#include <ArduinoJson.h>      // JSON for status publishing

// External references to global variables from main.cpp
extern PubSubClient mqttClient;  // MQTT client instance
extern bool provisioningActive;  // Flag for provisioning state

// BLE characteristics for WiFi provisioning
// These are GATT characteristics that apps can write to/read from.
BLECharacteristic *pSSIDCharacteristic;    // Characteristic for SSID input
BLECharacteristic *pPassCharacteristic;    // Characteristic for password input
BLECharacteristic *pStatusCharacteristic;  // Characteristic for status notifications

// Callback class for handling writes to characteristics
// Inherits from BLECharacteristicCallbacks to override onWrite method.
class ProvisioningCallbacks : public BLECharacteristicCallbacks {
    // Called when a BLE app writes to a characteristic
    void onWrite(BLECharacteristic *pCharacteristic) {
        // Get the written value as a string
        std::string value = pCharacteristic->getValue();
        if (pCharacteristic == pSSIDCharacteristic) {
            // SSID received: store temporarily (in production, save to NVS)
            Serial.printf("Received SSID: %s\n", value.c_str());
            // Note: Actual storage should use Preferences or NVS for persistence
        } else if (pCharacteristic == pPassCharacteristic) {
            // Password received: attempt WiFi connection
            Serial.printf("Received Password: %s\n", value.c_str());
            // Connect to WiFi using received SSID and password
            WiFi.begin(value.c_str(), pSSIDCharacteristic->getValue().c_str());
            // Wait for connection
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
            }
            Serial.println("WiFi connected via BLE provisioning");
            // Notify BLE app of success
            pStatusCharacteristic->setValue("Connected");
            pStatusCharacteristic->notify();
            // Stop provisioning
            provisioningActive = false;
            BLEDevice::deinit();  // Clean up BLE to save power
            // Publish WiFi connection status via MQTT
            DynamicJsonDocument doc(256);
            doc["wifiState"] = "connected";
            doc["ssid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            String msg;
            serializeJson(doc, msg);
            char topic[64];
            snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", MACHINE_ID);
            mqttClient.publish(topic, msg.c_str(), true);  // Retained publish for machine status
        }
    }
};

// Global BLE server instance
BLEServer* pServer = NULL;

// Callback class for BLE server events (connect/disconnect)
class MyServerCallbacks : public BLEServerCallbacks {
    // Called when a BLE client connects
    void onConnect(BLEServer* pServer) {
        Serial.println("BLE: Device connected");
    }
    // Called when a BLE client disconnects
    void onDisconnect(BLEServer* pServer) {
        Serial.println("BLE: Device disconnected");
    }
};

// Initialize BLE for provisioning
// Sets up GATT service with characteristics for SSID, password, and status.
void ble_init(const char* deviceName) {
    // Initialize BLE device with given name
    BLEDevice::init(deviceName);
    // Create BLE server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());  // Set event callbacks

    // Create GATT service for provisioning (custom UUID)
    BLEService *pService = pServer->createService("12345678-1234-1234-1234-123456789abc");

    // Characteristic for SSID (write-only)
    pSSIDCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-cba987654321",  // Custom UUID
        BLECharacteristic::PROPERTY_WRITE       // Allow writes
    );
    pSSIDCharacteristic->setCallbacks(new ProvisioningCallbacks());  // Attach callbacks

    // Characteristic for password (write-only)
    pPassCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-dba987654321",  // Custom UUID
        BLECharacteristic::PROPERTY_WRITE       // Allow writes
    );
    pPassCharacteristic->setCallbacks(new ProvisioningCallbacks());  // Attach callbacks

    // Characteristic for status (read and notify)
    pStatusCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-eba987654321",  // Custom UUID
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY  // Read and notify
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());  // Enable notifications

    // Start the service
    pService->start();

    // Setup advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("12345678-1234-1234-1234-123456789abc");  // Advertise service
    pAdvertising->setScanResponse(true);  // Allow scan responses
    pAdvertising->setMinPreferred(0x06);  // iPhone compatibility
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();  // Start advertising

    Serial.println("BLE provisioning started");
}

// Set BLE device name (if server exists)
void ble_set_device_name(const char* deviceName) {
    if (pServer) {
        pServer->setDeviceName(deviceName);
    }
}

// Start BLE provisioning
// Calls ble_init and sets provisioning flag.
void startBLEProvisioning() {
    ble_init("WATERFRONT-PROV");  // Initialize with provisioning name
    provisioningActive = true;    // Indicate provisioning is running
}
