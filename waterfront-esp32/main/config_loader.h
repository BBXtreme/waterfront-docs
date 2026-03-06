#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <cJSON.h>
#include "config.h"

// JSON buffer size for cJSON documents (tune if config grows)
#define JSON_BUFFER_SIZE 4096

// Thread-safety mutex for g_config access (ESP32 multi-core)
extern portMUX_TYPE g_configMutex;

// Global config instance - shared across the application
extern GlobalConfig g_config;

// MQTT config
struct MqttConfig {
    char broker[64];
    int port;
    char username[32];
    char password[32];
    char clientIdPrefix[32];
    bool useTLS;
    char caCertPath[64];
    char clientCertPath[64];
    char clientKeyPath[64];
};

// Location config
struct LocationConfig {
    char slug[32];
    char code[32];
};

// WiFi provisioning config
struct WifiProvisioningConfig {
    char fallbackSsid[32];
    char fallbackPass[32];
};

// LTE config
struct LteConfig {
    char apn[32];
    char simPin[16];
    int rssiThreshold;
    int dataUsageAlertLimitKb;
};

// BLE config
struct BleConfig {
    char serviceUuid[37];  // UUID length
    char ssidCharUuid[37];
    char passCharUuid[37];
    char statusCharUuid[37];
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
    int logLevel;
    char otaPassword[32];  // OTA password for security
};

// Other config
struct OtherConfig {
    int offlinePinTtlSec;
    int depositHoldAmountFallback;
};

// Global config struct
struct GlobalConfig {
    char version[16];  // Config version for migration
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
const char* getConfigAsJson();
bool validateConfig(const GlobalConfig& cfg);

#endif // CONFIG_LOADER_H
