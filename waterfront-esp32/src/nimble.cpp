// Adapted from original nimble.c, simplified for provisioning
#include "nimble.h"
#include "config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>  // For notifications if needed
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Extern MQTT client (assume defined in main.cpp)
extern PubSubClient mqttClient;
extern bool provisioningActive;

// BLE characteristics for WiFi provisioning
BLECharacteristic *pSSIDCharacteristic;
BLECharacteristic *pPassCharacteristic;
BLECharacteristic *pStatusCharacteristic;

class ProvisioningCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();
        if (pCharacteristic == pSSIDCharacteristic) {
            // Store SSID
            Serial.printf("Received SSID: %s\n", value.c_str());
            // Save to NVS or global var (simplified)
        } else if (pCharacteristic == pPassCharacteristic) {
            // Store password
            Serial.printf("Received Password: %s\n", value.c_str());
            // Attempt to connect
            WiFi.begin(value.c_str(), pSSIDCharacteristic->getValue().c_str());
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
            }
            Serial.println("WiFi connected via BLE provisioning");
            pStatusCharacteristic->setValue("Connected");
            pStatusCharacteristic->notify();
            // Stop provisioning
            provisioningActive = false;
            BLEDevice::deinit();
            // Publish status
            DynamicJsonDocument doc(256);
            doc["wifiState"] = "connected";
            doc["ssid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            String msg;
            serializeJson(doc, msg);
            mqttClient.publish(MQTT_STATUS_TOPIC, msg.c_str());
        }
    }
};

BLEServer* pServer = NULL;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        Serial.println("BLE: Device connected");
    }
    void onDisconnect(BLEServer* pServer) {
        Serial.println("BLE: Device disconnected");
    }
};

void ble_init(const char* deviceName) {
    BLEDevice::init(deviceName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create service for provisioning
    BLEService *pService = pServer->createService("12345678-1234-1234-1234-123456789abc");

    // SSID characteristic
    pSSIDCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-cba987654321",
        BLECharacteristic::PROPERTY_WRITE
    );
    pSSIDCharacteristic->setCallbacks(new ProvisioningCallbacks());

    // Password characteristic
    pPassCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-dba987654321",
        BLECharacteristic::PROPERTY_WRITE
    );
    pPassCharacteristic->setCallbacks(new ProvisioningCallbacks());

    // Status characteristic
    pStatusCharacteristic = pService->createCharacteristic(
        "87654321-4321-4321-4321-eba987654321",
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pStatusCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID("12345678-1234-1234-1234-123456789abc");
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("BLE provisioning started");
}

void ble_set_device_name(const char* deviceName) {
    if (pServer) {
        pServer->setDeviceName(deviceName);
    }
}

// Start BLE provisioning
void startBLEProvisioning() {
    ble_init("WATERFRONT-PROV");
    provisioningActive = true;
}
