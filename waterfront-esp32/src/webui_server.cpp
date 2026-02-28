// Adapted from original webui_server.c, simplified for provisioning
#include "webui_server.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// Extern MQTT client
extern PubSubClient mqttClient;
extern bool provisioningActive;

WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", "<h1>WiFi Setup</h1><form action='/set' method='POST'>SSID: <input name='ssid'><br>Pass: <input name='pass'><br><input type='submit'></form>");
}

void handleSet() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    // Save to config (simplified)
    Serial.printf("Received SSID: %s, Pass: %s\n", ssid.c_str(), pass.c_str());
    WiFi.begin(ssid.c_str(), pass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("WiFi connected via SoftAP provisioning");
    server.send(200, "text/plain", "WiFi set, rebooting...");
    provisioningActive = false;
    // Publish status
    DynamicJsonDocument doc(256);
    doc["wifiState"] = "connected";
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    String msg;
    serializeJson(doc, msg);
    mqttClient.publish(MQTT_STATUS_TOPIC, msg.c_str());
    ESP.restart();
}

void start_softap() {
    WiFi.softAP("WATERFRONT-PROV", "password123");  // Simple password for demo
    Serial.println("SoftAP started: WATERFRONT-PROV, password: password123");
    // In hardware, blink LED to indicate
}

void start_rest_server() {
    server.on("/", handleRoot);
    server.on("/set", HTTP_POST, handleSet);
    server.begin();
}

// Start SoftAP provisioning
void startSoftAPProvisioning() {
    start_softap();
    start_rest_server();
    provisioningActive = true;
}

void stop_rest_server() {
    server.stop();
}
