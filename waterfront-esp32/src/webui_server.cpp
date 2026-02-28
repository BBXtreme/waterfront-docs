// webui_server.cpp - Web server for SoftAP WiFi provisioning
// This file implements a simple HTTP server for SoftAP-based provisioning.
// When BLE is not available, the ESP32 creates a WiFi AP and serves a web page for SSID/password input.
// The server handles GET (form display) and POST (credential submission) requests.
// After successful connection, it publishes status and restarts the ESP32.

#include "webui_server.h"     // Header for web server functions
#include "config.h"            // Configuration constants
#include <WiFi.h>              // WiFi library for AP mode
#include <WebServer.h>         // Arduino web server library
#include <PubSubClient.h>      // MQTT client for status publishing
#include <ArduinoJson.h>       // JSON for status messages
#include <Preferences.h>       // For persistent WiFi credentials

// External references from main.cpp
extern PubSubClient mqttClient;  // MQTT client instance
extern bool provisioningActive;  // Provisioning state flag

// Global web server instance on port 80
WebServer server(80);

// Provisioning state machine
enum ProvState { PROV_IDLE, PROV_CONNECTING, PROV_SUCCESS, PROV_FAILED, PROV_TIMEOUT };
ProvState provState = PROV_IDLE;
unsigned long provStartTime = 0;
#define PROV_TIMEOUT_MS 30000  // 30 seconds timeout

// Preferences for storing WiFi credentials
Preferences preferences;

// Handler for root GET request: serves the WiFi setup form
void handleRoot() {
    // Simple HTML form for SSID and password input
    server.send(200, "text/html", "<h1>WiFi Setup</h1><form action='/set' method='POST'>SSID: <input name='ssid'><br>Pass: <input name='pass'><br><input type='submit'></form>");
}

// Handler for /set POST request: processes form submission
void handleSet() {
    // Extract SSID and password from POST data
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    // Log received credentials (for debugging)
    Serial.printf("Received SSID: %s, Pass: %s\n", ssid.c_str(), pass.c_str());
    // Store in Preferences and start connecting
    preferences.begin("wifi", false);
    preferences.putString("ssid", ssid.c_str());
    preferences.putString("pass", pass.c_str());
    preferences.end();
    // Start non-blocking WiFi connection
    provState = PROV_CONNECTING;
    provStartTime = millis();
    // Blocking version commented out for reference:
    // WiFi.begin(ssid.c_str(), pass.c_str());
    // while (WiFi.status() != WL_CONNECTED) {
    //     delay(500);
    // }
    // Serial.println("WiFi connected via SoftAP provisioning");
    // server.send(200, "text/plain", "WiFi set, rebooting...");
    // provisioningActive = false;
    // DynamicJsonDocument doc(256);
    // doc["wifiState"] = "connected";
    // doc["ssid"] = WiFi.SSID();
    // doc["ip"] = WiFi.localIP().toString();
    // String msg;
    // serializeJson(doc, msg);
    // mqttClient.publish("waterfront/machine/" SLOT_ID "/status", msg.c_str(), true);
    // ESP.restart();
}

// Provisioning task (call from main loop)
void provisioning_task() {
    if (provState == PROV_CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            // Success
            provState = PROV_SUCCESS;
            Serial.println("WiFi connected via SoftAP provisioning");
            // Send success response to client
            server.send(200, "text/plain", "WiFi set, rebooting...");
            // Stop provisioning
            provisioningActive = false;
            // Publish WiFi connection status via MQTT
            DynamicJsonDocument doc(256);
            doc["wifiState"] = "connected";
            doc["ssid"] = WiFi.SSID();
            doc["ip"] = WiFi.localIP().toString();
            String msg;
            serializeJson(doc, msg);
            char topic[64];
            snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", MACHINE_ID);
            mqttClient.publish(topic, msg.c_str(), true);  // Retained publish for machine status
            // Reconnect MQTT if needed
            mqtt_connect();
            // Restart ESP32 to apply changes
            ESP.restart();
        } else if (millis() - provStartTime > PROV_TIMEOUT_MS) {
            // Timeout: fallback to previous creds or error publish
            provState = PROV_TIMEOUT;
            Serial.println("WiFi connection timeout via SoftAP provisioning, falling back to previous credentials");
            // Load previous credentials from NVS
            preferences.begin("wifi", true);  // Read-only
            String prevSSID = preferences.getString("ssid", "");
            String prevPass = preferences.getString("pass", "");
            preferences.end();
            if (prevSSID.length() > 0 && prevPass.length() > 0) {
                Serial.printf("Trying previous creds: SSID=%s\n", prevSSID.c_str());
                WiFi.begin(prevSSID.c_str(), prevPass.c_str());
                provState = PROV_CONNECTING;  // Retry with previous
                provStartTime = millis();
            } else {
                // No previous creds, fail
                provState = PROV_FAILED;
                Serial.println("No previous credentials, provisioning failed");
                // Send failure response
                server.send(200, "text/plain", "WiFi connection failed, try again");
                // Publish error via MQTT
                DynamicJsonDocument doc(256);
                doc["wifiState"] = "failed";
                doc["error"] = "timeout_no_fallback";
                String msg;
                serializeJson(doc, msg);
                char topic[64];
                snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", MACHINE_ID);
                mqttClient.publish(topic, msg.c_str(), true);
                // Reset state
                provState = PROV_IDLE;
            }
        }
    }
}

// Start SoftAP mode with provisioning SSID and password
void start_softap() {
    // Configure AP with fixed SSID/password (for demo; randomize in production)
    WiFi.softAP("WATERFRONT-PROV", "password123");
    Serial.println("SoftAP started: WATERFRONT-PROV, password: password123");
    // In production, blink LED or display password on hardware
}

// Start the REST server and define routes
void start_rest_server() {
    // Route for root page (form)
    server.on("/", handleRoot);
    // Route for form submission
    server.on("/set", HTTP_POST, handleSet);
    // Start server
    server.begin();
}

// Start SoftAP provisioning
// Calls functions to setup AP and server, sets provisioning flag.
void startSoftAPProvisioning() {
    start_softap();           // Start AP mode
    start_rest_server();     // Start web server
    provisioningActive = true;  // Indicate provisioning active
}

// Stop the REST server
void stop_rest_server() {
    server.stop();  // Stop listening for requests
}
