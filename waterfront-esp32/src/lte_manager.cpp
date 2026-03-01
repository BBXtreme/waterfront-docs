#include <TinyGsmClient.h>
#include <PubSubClient.h>
#include "mqtt_client.h"
#include "config_loader.h"

TinyGsm modem(SerialAT);
TinyGsmClient lteClient(modem);

void lte_init() {
    SerialAT.begin(115200, SERIAL_8N1, 16, 17);  // Use config pins if needed
    pinMode(25, OUTPUT);  // PWRKEY
    digitalWrite(25, LOW);
    ESP_LOGI("LTE", "Initialized (powered off)");
}

void lte_power_up() {
    digitalWrite(25, HIGH);
    delay(1000);
    digitalWrite(25, LOW);
    delay(10000);
    modem.restart();
    modem.gprsConnect(g_config.lte.apn.c_str(), "", "");
    ESP_LOGI("LTE", "Powered up and connected to GPRS");
}

void lte_power_down() {
    modem.poweroff();
    digitalWrite(25, LOW);
    delay(1000);  // Ensure power down
    ESP_LOGI("LTE", "Powered down");
}

void lte_switch_to_lte() {
    lte_power_up();
    mqttClient.setClient(lteClient);
    if (mqttClient.connect((g_config.mqtt.clientIdPrefix + "-lte").c_str())) {
        ESP_LOGI("LTE", "MQTT switched to LTE");
    } else {
        ESP_LOGE("LTE", "MQTT connect failed over LTE");
    }
}

void lte_switch_to_wifi() {
    mqttClient.setClient(espClient);
    ESP_LOGI("LTE", "MQTT switched back to WiFi");
    lte_power_down();
}

int lte_get_signal() {
    return modem.getSignalQuality();
}

bool lte_is_connected() {
    return modem.isGprsConnected();
}

float readSolarVoltage() {
    // Assume ADC pin from config (e.g., GPIO 35)
    int adcValue = analogRead(35);  // ADC pin for solar
    if (adcValue < 0 || adcValue > 4095) {
        ESP_LOGE("LTE", "Invalid ADC value for solar: %d", adcValue);
        return 0.0f;  // Fallback
    }
    // Convert to voltage (calibrate based on divider)
    float voltage = (adcValue / 4095.0) * 3.3 * (10.0 / 3.3);  // Example divider 10k/3.3k
    return voltage;
}

bool shouldDisableLTE() {
    float solarVoltage = readSolarVoltage();
    return solarVoltage < g_config.system.solarVoltageMin;
}

void lte_power_management() {
    // Power down LTE if WiFi is connected and MQTT has been idle for >5 minutes
    if (WiFi.status() == WL_CONNECTED && mqttClient.connected() && (millis() - lastMqttActivity > 300000)) {  // 5 min idle
        lte_power_down();
        ESP_LOGI("LTE", "Powered down due to WiFi connected and idle > 5 min");
    }
    // Power down LTE if solar voltage is low to conserve power
    if (shouldDisableLTE()) {
        lte_power_down();
        ESP_LOGI("LTE", "Powered down due to low solar voltage");
    }
}
