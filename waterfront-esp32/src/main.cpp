#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "provisioning.h"
#include "webui_server.h"
#include "lte_manager.h"

// Global MQTT client
PubSubClient mqttClient(wifiClient);
WiFiClient wifiClient;
bool provisioningActive = false;

// Connectivity state
enum ConnectivityState { WIFI, LTE, OFFLINE };
ConnectivityState currentConn = WIFI;

// Function prototypes
void setupWiFi();
void setupMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT();
void publishStatus();
void checkProvisioningButton();
void checkConnectivity();

// Tasks
TaskHandle_t mqttTaskHandle;

void setup() {
  Serial.begin(115200);
  Serial.println("WATERFRONT-ESP32: Hello from Waterfront ESP32 Kayak Controller!");
  printf("Build successful - base ESP-IDF setup ready for MQTT integration.\n");

  pinMode(PROVISIONING_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_LED_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);

  // Initialize LTE modem (power off initially)
  initLTE();

  // Check if WiFi provisioned (simplified, check NVS or hardcoded)
  if (WiFi.SSID() == "") {  // Not provisioned
    startBLEProvisioning();  // Try BLE first
  } else {
    setupWiFi();
    setupMQTT();
  }

  xTaskCreate(mqttTask, "MQTT", 4096, NULL, 1, &mqttTaskHandle);
}

void loop() {
  checkProvisioningButton();
  if (provisioningActive) {
    // Handle SoftAP server if active
    server.handleClient();
  }
  checkConnectivity();
  delay(100);
}

// WiFi setup
void setupWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("WiFi connected");
  currentConn = WIFI;
}

// MQTT setup
void setupMQTT() {
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

// MQTT callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  if (strcmp(topic, MQTT_UNLOCK_TOPIC) == 0) {
    DynamicJsonDocument doc(256);
    deserializeJson(doc, msg);
    String pin = doc["pin"];
    if (pin == "1234") {
      digitalWrite(RELAY_PIN, HIGH);
      delay(RELAY_PULSE_DURATION_MS);
      digitalWrite(RELAY_PIN, LOW);
    }
  } else if (strcmp(topic, MQTT_PROVISION_TOPIC) == 0) {
    startBLEProvisioning();
  }
}

// MQTT task
void mqttTask(void* pvParameters) {
  while (true) {
    if (!mqttClient.connected()) {
      reconnectMQTT();
    }
    mqttClient.loop();
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

// Reconnect MQTT
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect(MACHINE_ID)) {
      mqttClient.subscribe(MQTT_UNLOCK_TOPIC);
      mqttClient.subscribe(MQTT_PROVISION_TOPIC);
      publishStatus();
    } else {
      delay(5000);
    }
  }
}

// Publish status
void publishStatus() {
  DynamicJsonDocument doc(256);
  doc["wifiState"] = "connected";
  doc["ssid"] = WiFi.SSID();
  doc["ip"] = WiFi.localIP().toString();
  doc["connType"] = (currentConn == WIFI) ? "WiFi" : (currentConn == LTE) ? "LTE" : "offline";
  doc["rssi"] = WiFi.RSSI();
  doc["dataKB"] = 0;  // Placeholder for data usage
  String msg;
  serializeJson(doc, msg);
  mqttClient.publish(MQTT_STATUS_TOPIC, msg.c_str());
}

// Provisioning button
void checkProvisioningButton() {
  static unsigned long pressStart = 0;
  static bool pressed = false;
  if (digitalRead(PROVISIONING_BUTTON_PIN) == LOW) {
    if (!pressed) {
      pressStart = millis();
      pressed = true;
    } else if (millis() - pressStart > 3000) {
      startBLEProvisioning();
      pressed = false;
    }
  } else {
    pressed = false;
  }
}

// Connectivity check and failover
void checkConnectivity() {
  static unsigned long wifiCheckStart = 0;
  if (currentConn == WIFI) {
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiCheckStart == 0) {
        wifiCheckStart = millis();
        Serial.println("WiFi disconnected, starting failover timer...");
      } else if (millis() - wifiCheckStart > WIFI_FAILOVER_TIMEOUT_MS) {
        Serial.println("WiFi failover timeout, switching to LTE...");
        switchToLTE();
      }
    } else {
      wifiCheckStart = 0;
      // Power down modem if on WiFi
      powerDownModem();
    }
  } else if (currentConn == LTE) {
    // Check if WiFi is back
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi back, switching from LTE...");
      switchToWiFi();
    }
  }
}
