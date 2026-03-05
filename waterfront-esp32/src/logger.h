#ifndef LOGGER_H
#define LOGGER_H

#include <esp_log.h>

// Initialize logger with config
void logger_init();

// Log to file if verbose
void logger_log_to_file(const char* tag, const char* message);

// Rotate log file if size > threshold
void logger_rotate_log_file();

#endif // LOGGER_H
