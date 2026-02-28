#include "mqtt_client.h"
#include "config.h"
#include <ArduinoJson.h>

// Adapted from original mqtt_event_handler in mdb-slave-esp32s3.c
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    ESP_LOGI("MQTT", "Message received: %s", message.c_str());

    // Parse JSON
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        ESP_LOGI("MQTT", "JSON parse failed");
        return;
    }
    const char* bookingId = doc["bookingId"];
    const char* pin = doc["pin"];
    ESP_LOGI("Unlock request", "booking %s pin %s", bookingId, pin);
}

void mqtt_init() {
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqtt_callback);
    if (mqttClient.connect("KayakClient")) {
        mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC);
        ESP_LOGI("MQTT", "Connected and subscribed");
    }
}

void mqtt_publish_status() {
    DynamicJsonDocument doc(256);
    doc["state"] = "idle";
    doc["battery"] = 92;
    doc["kayakPresent"] = true;
    doc["connType"] = "wifi";
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(MQTT_PUBLISH_TOPIC, payload.c_str());
}
