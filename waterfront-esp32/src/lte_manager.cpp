// lte_manager.cpp - LTE cellular fallback management using TinyGSM
// This file manages the LTE modem for cellular connectivity as a WiFi fallback.
// It uses TinyGSM library to control the modem, connect to GPRS, and provide TCP client for MQTT.
// Power management is included to conserve battery in solar-powered setups.

#include "lte_manager.h"
#include "config.h"
#include <TinyGsmClient.h>
#include <HardwareSerial.h>
#include <PubSubClient.h>

// External references
extern PubSubClient mqttClient;

// TinyGSM setup
HardwareSerial SerialAT(2);
TinyGsm modem(SerialAT);
TinyGsmClient lteClient(modem);

// Initialize LTE modem
void lte_init() {
    SerialAT.begin(LTE_MODEM_BAUD, SERIAL_8N1, LTE_MODEM_RX_PIN, LTE_MODEM_TX_PIN);
    pinMode(LTE_MODEM_PWRKEY_PIN, OUTPUT);
    digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);
    ESP_LOGI("LTE", "Initialized (powered off)");
}

// Power up and connect modem
void lte_power_up() {
    digitalWrite(LTE_MODEM_PWRKEY_PIN, HIGH);
    delay(1000);
    digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);
    delay(10000);
    modem.restart();
    modem.gprsConnect(LTE_APN, "", "");
    ESP_LOGI("LTE", "Powered up and connected to GPRS");
}

// Power down modem
void lte_power_down() {
    modem.poweroff();
    digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);
    ESP_LOGI("LTE", "Powered down");
}

// Switch MQTT to LTE
void lte_switch_to_lte() {
    lte_power_up();
    mqttClient.setClient(lteClient);
    if (mqttClient.connect(MACHINE_ID)) {
        ESP_LOGI("LTE", "MQTT switched to LTE");
    } else {
        ESP_LOGE("LTE", "MQTT connect failed over LTE");
    }
}

// Switch MQTT back to WiFi
void lte_switch_to_wifi() {
    mqttClient.setClient(wifiClient);
    ESP_LOGI("LTE", "MQTT switched back to WiFi");
    lte_power_down();
}

// Check LTE signal
int lte_get_signal() {
    return modem.getSignalQuality();
}

// Is LTE connected
bool lte_is_connected() {
    return modem.isGprsConnected();
}
