// lte_manager.cpp - LTE cellular fallback using TinyGSM

#include "lte_manager.h"
#include "config.h"
#include <PubSubClient.h>

// Global MQTT client (extern from main)
extern PubSubClient mqttClient;

// TinyGSM setup
HardwareSerial SerialAT(2);  // UART2
TinyGsm modem(SerialAT);
TinyGsmClient lteClient(modem);

// Connectivity state (extern from main)
extern ConnectivityState currentConn;

void initLTE() {
  SerialAT.begin(LTE_MODEM_BAUD, SERIAL_8N1, LTE_MODEM_RX_PIN, LTE_MODEM_TX_PIN);
  pinMode(LTE_MODEM_PWRKEY_PIN, OUTPUT);
  digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);  // Start powered off
  Serial.println("LTE modem initialized (powered off)");
}

void powerUpModem() {
  digitalWrite(LTE_MODEM_PWRKEY_PIN, HIGH);
  delay(1000);  // PWRKEY pulse
  digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);
  Serial.println("LTE modem powered up");
  // Wait for modem to boot
  delay(10000);
  modem.restart();
  modem.gprsConnect(LTE_APN, "", "");
  Serial.println("LTE GPRS connected");
}

void powerDownModem() {
  modem.poweroff();
  digitalWrite(LTE_MODEM_PWRKEY_PIN, LOW);
  Serial.println("LTE modem powered down");
}

void switchToLTE() {
  powerUpModem();
  // Switch MQTT client to LTE
  mqttClient.setClient(lteClient);
  currentConn = LTE;
  Serial.println("Switched to LTE connectivity");
  // Reconnect MQTT over LTE
  reconnectMQTT();
}

void switchToWiFi() {
  // Switch MQTT client back to WiFi
  mqttClient.setClient(wifiClient);
  currentConn = WIFI;
  Serial.println("Switched to WiFi connectivity");
  // Reconnect MQTT over WiFi
  reconnectMQTT();
}
