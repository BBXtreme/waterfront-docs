#include "logger.h"
#include "config_loader.h"
#include <esp_spiffs.h>
#include <esp_log.h>

#define LOG_FILE "/spiffs/system.log"
#define LOG_MAX_SIZE 102400  // 100KB

// Initialize logger
void logger_init() {
    // Set log level from config
    esp_log_level_set("*", (esp_log_level_t)g_config.system.logLevel);
    ESP_LOGI("LOGGER", "Logger initialized with level %d", g_config.system.logLevel);
}

// Log to file if verbose (logLevel >= INFO)
void logger_log_to_file(const char* tag, const char* message) {
    if (g_config.system.logLevel >= ESP_LOG_INFO) {
        FILE *logFile = fopen(LOG_FILE, "a");
        if (logFile) {
            fprintf(logFile, "[%s] %s\n", tag, message);
            fclose(logFile);
            // Check size and rotate
            struct stat st;
            if (stat(LOG_FILE, &st) == 0 && st.st_size > LOG_MAX_SIZE) {
                logger_rotate_log_file();
            }
        }
    }
}

// Rotate log file
void logger_rotate_log_file() {
    if (esp_spiffs_rename(LOG_FILE, LOG_FILE ".1") == ESP_OK) {
        ESP_LOGI("LOGGER", "Log file rotated");
    } else {
        ESP_LOGE("LOGGER", "Failed to rotate log file");
    }
}
