/**
 * @file error_handler.cpp
 * @brief Global error handling implementation for WATERFRONT ESP32.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Logs fatal errors and restarts ESP32.
 */

#include "error_handler.h"
#include <esp_log.h>
#include <esp_system.h>
#include <Arduino.h>

void fatal_error(const char* msg, esp_err_t code) {
    ESP_LOGE("FATAL", "%s: %s (0x%x)", msg, esp_err_to_name(code), code);
    delay(5000);
    esp_restart();
}
