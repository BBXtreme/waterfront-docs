#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/portmacro.h>
#include <cJSON.h>
#include "config.h"

#define JSON_BUFFER_SIZE 4096

extern portMUX_TYPE g_configMutex;
extern GlobalConfig g_config;

bool loadConfig();
bool saveConfig();
bool updateConfigFromJson(const char* jsonPayload);
GlobalConfig getDefaultConfig();
const char* getConfigAsJson();
bool validateConfig(const GlobalConfig& cfg);

#endif