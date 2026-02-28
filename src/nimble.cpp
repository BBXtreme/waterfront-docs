// Adapted from original nimble.c, simplified for provisioning
#include "nimble.h"
#include "config.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

BLEServer* pServer = NULL;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        ESP_LOGI("BLE", "Device connected");
    }
    void onDisconnect(BLEServer* pServer) {
        ESP_LOGI("BLE", "Device disconnected");
    }
};

void ble_init(const char* deviceName) {
    BLEDevice::init(deviceName);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    // Add GATT services as needed (simplified)
}

void ble_set_device_name(const char* deviceName) {
    if (pServer) {
        pServer->setDeviceName(deviceName);
    }
}
