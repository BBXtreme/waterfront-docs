// Adapted from original webui_server.c, simplified for provisioning
#include "webui_server.h"
#include "config.h"
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

void handleRoot() {
    server.send(200, "text/html", "<h1>WiFi Setup</h1><form action='/set' method='POST'>SSID: <input name='ssid'><br>Pass: <input name='pass'><br><input type='submit'></form>");
}

void handleSet() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    // Save to config (simplified)
    server.send(200, "text/plain", "WiFi set, rebooting...");
    ESP.restart();
}

void start_softap() {
    WiFi.softAP("KayakAP", "password");
    ESP_LOGI("SoftAP", "Started");
}

void start_rest_server() {
    server.on("/", handleRoot);
    server.on("/set", HTTP_POST, handleSet);
    server.begin();
}

void stop_rest_server() {
    server.stop();
}
