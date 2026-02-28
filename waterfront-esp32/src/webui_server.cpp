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

// External references from main.cpp
extern PubSubClient mqttClient;  // MQTT client instance
extern bool provisioningActive;  // Provisioning state flag

// Global web server instance on port 80
WebServer server(80);

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
    // Attempt WiFi connection
    WiFi.begin(ssid.c_str(), pass.c_str());
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("WiFi connected via SoftAP provisioning");
    // Send success response to client
    server.send(200, "text/plain", "WiFi set, rebooting...");
    // Stop provisioning
    provisioningActive = false;
    // Publish connection status via MQTT
    DynamicJsonDocument doc(256);
    doc["wifiState"] = "connected";
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    String msg;
    serializeJson(doc, msg);
    char topic[64];
    snprintf(topic, sizeof(topic), "waterfront/machine/%s/status", MACHINE_ID);
    mqttClient.publish(topic, msg.c_str(), true);  // Retained publish for machine status
    // Restart ESP32 to apply changes
    ESP.restart();
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
