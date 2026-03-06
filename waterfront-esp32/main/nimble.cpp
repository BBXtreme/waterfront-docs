// nimble.cpp - BLE (Bluetooth Low Energy) provisioning implementation
// This file handles BLE-based WiFi provisioning for the ESP32.
// It uses the Arduino BLE library (NimBLE) to advertise a GATT service for SSID/password input.
// When credentials are received, it connects to WiFi and publishes status via MQTT.
// This is the preferred provisioning method (secure, works with apps like nRF Connect).

#include "nimble.h"
#include "config_loader.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// External references to global variables from main.cpp
extern PubSubClient mqttClient;  // MQTT client instance
extern bool provisioningActive;  // Flag for provisioning state

// BLE characteristics for WiFi provisioning
// These are GATT characteristics that apps can write to/read from.
BLECharacteristic *pSSIDCharacteristic;    // Characteristic for SSID input
BLECharacteristic *pPassCharacteristic;    // Characteristic for password input
BLECharacteristic *pStatusCharacteristic;  // Characteristic for status notifications

// Provisioning state machine
enum ProvState { PROV_IDLE, PROV_CONNECTING, PROV_SUCCESS, PROV_FAILED, PROV_TIMEOUT };
ProvState provState = PROV_IDLE;
unsigned long provStartTime = 0;
#define PROV_TIMEOUT_MS 30000  // 30 seconds timeout

// Preferences for storing WiFi credentials
Preferences preferences;

// Callback class for handling writes to characteristics
// Inherits from BLECharacteristicCallbacks to override onWrite method.
class ProvisioningCallbacks : public BLECharacteristicCallbacks {
    // Called when a BLE app writes to a characteristic
    void onWrite(BLECharacteristic *pCharacteristic) {
        // Get the written value as a string
        std::string value = pCharacteristic->getValue();
        if (pCharacteristic == pSSIDCharacteristic) {
            // SSID received: store in Preferences
            preferences.begin("wifi", false);
            preferences.putString("ssid", value.c_str());
            preferences.end();
            Serial.printf("Received SSID: %s\n", value.c_str());
        } else if (pCharacteristic == pPassCharacteristic) {
            // Password received: store in Preferences and start connecting
            preferences.begin("wifi", false);
            preferences.putString("pass", value.c_str());
            preferences.end();
            Serial.printf("Received Password: %s\n", value.c_str());
            // Start non-blocking WiFi connection
            provState = PROV_CONNECTING;
            provStartTime = millis();
            // Blocking version commented out for reference:
            // WiFi.begin(value.c_str(), pSSIDCharacteristic->getValue().c_str());
            // while (WiFi.status() != WL_CONNECTED) {
            //     delay(500);
            // }
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
    BLEService *pService = pServer->createService(g_config.ble.serviceUuid.c_str());

    // Characteristic for SSID (write-only)
    pSSIDCharacteristic = pService->createCharacteristic(
        g_config.ble.ssidCharUuid.c_str(),  // Custom UUID
        BLECharacteristic::PROPERTY_WRITE       // Allow writes
    );
    pSSIDCharacteristic->setCallbacks(new ProvisioningCallbacks());  // Attach callbacks

    // Characteristic for password (write-only)
    pPassCharacteristic = pService->createCharacteristic(
        g_config.ble.passCharUuid.c_str(),  // Custom UUID
        BLECharacteristic::PROPERTY_WRITE       // Allow writes
    );
    pPassCharacteristic->setCallbacks(new ProvisioningCallbacks());  // Attach callbacks

    // Characteristic for status (read and notify)
    pStatusCharacteristic = pService->createCharacteristic(
        g_config.ble.statusCharUuid.c_str(),  // Custom UUID
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY  // Read and notify
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());  // Enable notifications

    // Start the service
    pService->start();

    // Setup advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(g_config.ble.serviceUuid.c_str());  // Advertise service
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

// Provisioning task (call from main loop)
void provisioning_task() {
    if (provState == PROV_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            // Success
            provState = PROV_SUCCESS;
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
            vPortEnterCritical(&g_configMutex);
            String locationCode = g_config.location.code;
            vPortExitCritical(&g_configMutex);
            snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", locationCode.c_str());
            mqttClient.publish(topic, msg.c_str(), true);  // Retained publish for machine status
            // Reconnect MQTT if needed
            mqtt_connect();
        } else if (millis() - provStartTime > PROV_TIMEOUT_MS) {
            // Timeout: fallback to previous creds or error publish
            provState = PROV_TIMEOUT;
            Serial.println("WiFi connection timeout via BLE provisioning, falling back to previous credentials");
            // Load previous credentials from NVS
            preferences.begin("wifi", true);  // Read-only
            String prevSSID = preferences.getString("ssid", "");
            String prevPass = preferences.getString("pass", "");
            preferences.end();
            if (prevSSID.length() > 0 && prevPass.length() > 0) {
                Serial.printf("Trying previous creds: SSID=%s\n", prevSSID.c_str());
                WiFi.begin(prevSSID.c_str(), prevPass.c_str());
                provState = PROV_CONNECTING;  // Retry with previous
                provStartTime = millis();
            } else {
                // No previous creds, fail
                provState = PROV_FAILED;
                Serial.println("No previous credentials, provisioning failed");
                // Notify BLE app of failure
                pStatusCharacteristic->setValue("Failed");
                pStatusCharacteristic->notify();
                // Publish error via MQTT
                DynamicJsonDocument doc(256);
                doc["wifiState"] = "failed";
                doc["error"] = "timeout_no_fallback";
                String msg;
                serializeJson(doc, msg);
                char topic[64];
                vPortEnterCritical(&g_configMutex);
                String locationCode = g_config.location.code;
                vPortExitCritical(&g_configMutex);
                mqttClient.publish(topic, msg.c_str(), true);
                // Reset state
                provState = PROV_IDLE;
            }
        }
    }
}

// Start BLE provisioning
// Calls ble_init and sets provisioning flag.
void startBLEProvisioning() {
    vPortEnterCritical(&g_configMutex);
    String locationSlug = g_config.location.slug;
    vPortExitCritical(&g_configMutex);
    ble_init(locationSlug.c_str());
    provisioningActive = true;    // Indicate provisioning is running
}
