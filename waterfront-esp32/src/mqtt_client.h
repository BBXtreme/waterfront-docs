#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <PubSubClient.h>
#include <WiFiClient.h>

extern WiFiClient espClient;
extern PubSubClient mqttClient;

void mqtt_init();
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void mqtt_publish_status();

#endif // MQTT_CLIENT_H
