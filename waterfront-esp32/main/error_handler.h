/**
 * @file error_handler.h
 * @brief Header for global error handling in WATERFRONT ESP32.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Provides fatal error function for production stability.
 */

#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <esp_err.h>

/**
 * @brief Handles fatal errors by logging and restarting ESP32.
 * @param msg Descriptive error message.
 * @param code ESP error code.
 */
void fatal_error(const char* msg, esp_err_t code);

#endif // ERROR_HANDLER_H
