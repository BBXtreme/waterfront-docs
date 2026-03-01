#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <Arduino.h>
#include <vector>
#include <ArduinoJson.h>

struct MqttConfig {
  String broker