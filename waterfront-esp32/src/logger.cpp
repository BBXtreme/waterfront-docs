#include "logger.h"
#include "config_loader.h"
#include <LittleFS.h>
#include <esp_log.h>

#define LOG_FILE "/littlefs/system.log"
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
        File logFile = LittleFS.open(LOG_FILE, "a");
        if (logFile) {
            logFile.printf("[%s] %s\n", tag, message);
            logFile.close();
            // Check size and rotate
            if (logFile.size() > LOG_MAX_SIZE) {
                logger_rotate_log_file();
            }
        }
    }
}

// Rotate log file
void logger_rotate_log_file() {
    if (LittleFS.exists(LOG_FILE ".1")) {
        LittleFS.remove(LOG_FILE ".1");
    }
    LittleFS.rename(LOG_FILE, LOG_FILE ".1");
    ESP_LOGI("LOGGER", "Log file rotated");
}
