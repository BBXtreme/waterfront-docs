#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <string>
#include <nlohmann/json.hpp>
#include "config.h"

// JSON buffer size for ArduinoJson documents (tune if config grows)
#define JSON_BUFFER_SIZE 4096

// Thread-safety mutex for g_config access (ESP32 multi-core)
extern portMUX_TYPE g_configMutex;

// MQTT config
struct MqttConfig {
    std::string broker;
    int port;
    std::string username;
    std::string password;
    std::string clientIdPrefix;
    bool useTLS;
    std::string caCertPath;
    std::string clientCertPath;
    std::string clientKeyPath;
    bool tlsSkipVerify;
};

// Location config
struct LocationConfig {
    std::string slug;
    std::string code;
};

// WiFi provisioning config
struct WifiProvisioningConfig {
    std::string fallbackSsid;
    std::string fallbackPass;
};

// LTE config
struct LteConfig {
    std::string apn;
    std::string simPin;
    int rssiThreshold;
    int dataUsageAlertLimitKb;
};

// BLE config
struct BleConfig {
    std::string serviceUuid;
    std::string ssidCharUuid;
    std::string passCharUuid;
    std::string statusCharUuid;
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
    std::string version;  // Config version for migration
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
std::string getConfigAsJson();
bool validateConfig(const GlobalConfig& cfg);

#endif // CONFIG_LOADER_H
