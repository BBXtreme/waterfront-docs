#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <cstring>

#define MAX_COMPARTMENTS 10

// Global config struct
struct GlobalConfig {
    char version[16];

    struct {
        char broker[64];
        int port;
        char username[32];
        char password[32];
        char clientIdPrefix[32];
        bool useTLS;
        char caCertPath[64];
        char clientCertPath[64];
        char clientKeyPath[64];
    } mqtt;

    struct {
        char slug[32];
        char code[32];
    } location;

    struct {
        char fallbackSsid[32];
        char fallbackPass[32];
    } wifiProvisioning;

    struct {
        char apn[32];
        char simPin[16];
        int rssiThreshold;
        int dataUsageAlertLimitKb;
    } lte;

    struct {
        char serviceUuid[37];
        char ssidCharUuid[37];
        char passCharUuid[37];
        char statusCharUuid[37];
    } ble;

    struct {
        int number;
        int servoPin;
        int limitOpenPin;
        int limitClosePin;
        int ultrasonicTriggerPin;
        int ultrasonicEchoPin;
        int weightSensorPin;
    } compartments[MAX_COMPARTMENTS];

    int compartmentCount;

    struct {
        int maxCompartments;
        bool debugMode;
        int gracePeriodSec;
        int batteryLowThresholdPercent;
        float solarVoltageMin;
        int logLevel;
        char otaPassword[32];
    } system;

    struct {
        int offlinePinTtlSec;
        int depositHoldAmountFallback;
    } other;
};

extern GlobalConfig g_config;
extern portMUX_TYPE g_configMutex;

#endif // CONFIG_H