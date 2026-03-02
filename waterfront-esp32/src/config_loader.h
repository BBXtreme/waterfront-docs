#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"

// JSON buffer size for ArduinoJson documents (tune if config grows)
#define JSON_BUFFER_SIZE 4096

// Thread-safety mutex for g_config access (ESP32 multi-core)
extern portMUX_TYPE g_configMutex;

// MQTT config
struct MqttConfig {
    String broker;
    int port;
    String username;
    String password;
    String clientIdPrefix;
    bool useTLS;
    String caCertPath;
    String clientCertPath;
    String clientKeyPath;
    bool tlsSkipVerify;
};

// Location config
struct LocationConfig {
    String slug;
    String code;
};

// WiFi provisioning config
struct WifiProvisioningConfig {
    String fallbackSsid;
    String fallbackPass;
};

// LTE config
struct LteConfig {
    String apn;
    String simPin;
    int rssiThreshold;
    int dataUsageAlertLimitKb;
};

// BLE config
struct BleConfig {
    String serviceUuid;
    String ssidCharUuid;
    String passCharUuid;
    String statusCharUuid;
};

// Compartment config (pins)
struct CompartmentConfig {
    int number;
    int servoPin;
    int limitOpenPin;
    int limitClosePin;
    int ultrasonicTriggerPin;
    int ultrasonicEchoPin;
    int weightSensorPin;
};

// System config
struct SystemConfig {
    int maxCompartments;
    bool debugMode;
    int gracePeriodSec;
    int batteryLowThresholdPercent;
    float solarVoltageMin;
};

// Other config
struct OtherConfig {
    int offlinePinTtlSec;
    int depositHoldAmountFallback;
};

// Global config struct
struct GlobalConfig {
    String version;  // Config version for migration
    MqttConfig mqtt;
    LocationConfig location;
    WifiProvisioningConfig wifiProvisioning;
    LteConfig lte;
    BleConfig ble;
    CompartmentConfig compartments[MAX_COMPARTMENTS];
    int compartmentCount;  // For compatibility
    SystemConfig system;
    OtherConfig other;
};

// Functions
bool loadConfig();
bool saveConfig();
bool updateConfigFromJson(const char* jsonPayload);
GlobalConfig getDefaultConfig();
String getConfigAsJson();
bool validateConfig(const GlobalConfig& cfg);

#endif // CONFIG_LOADER_H
