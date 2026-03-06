#include "deposit_logic.h"
#include "config_loader.h"
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <esp_log.h>
#include <cJSON.h>
#include <mqtt_client.h>

RentalTimer activeTimers[MAX_TIMERS];
int activeTimersCount = 0;
bool deposit_held = false;
unsigned long rental_start_time = 0;
unsigned long rental_duration_ms = 0;

// Mutex for thread-safe access to deposit logic state
portMUX_TYPE depositMutex = portMUX_INITIALIZER_UNLOCKED;

// Overdue callback for FreeRTOS timer
void overdueCallback(TimerHandle_t xTimer) {
    int compartmentId = (int)pvTimerGetTimerID(xTimer);
    ESP_LOGI("DEPOSIT", "Compartment %d overdue, auto-locking", compartmentId);
    // Auto-lock: close the gate (assuming gate_control is available)
    // TODO: Integrate with gate_control.h to close the gate
    // For now, log and remove from activeTimers
    for (int i = 0; i < activeTimersCount; ) {
        if (activeTimers[i].compartmentId == compartmentId) {
            xTimerDelete(activeTimers[i].timerHandle, 0);
            // Shift array
            for (int j = i; j < activeTimersCount - 1; j++) {
                activeTimers[j] = activeTimers[j + 1];
            }
            activeTimersCount--;
            break;
        } else {
            ++i;
        }
    }
    // Publish overdue event if MQTT available
    // TODO: Publish MQTT message for overdue
}

void deposit_init() {
    vPortEnterCritical(&depositMutex);
    deposit_held = false;
    activeTimersCount = 0;
    vPortExitCritical(&depositMutex);
    ESP_LOGI("DEPOSIT", "Initialized");
}

void startRental(int compartmentId, unsigned long durationSec) {
    // Check if already active
    vPortEnterCritical(&depositMutex);
    for (int i = 0; i < activeTimersCount; i++) {
        if (activeTimers[i].compartmentId == compartmentId) {
            ESP_LOGW("DEPOSIT", "Rental already active for compartment %d", compartmentId);
            vPortExitCritical(&depositMutex);
            return;
        }
    }

    if (activeTimersCount >= MAX_TIMERS) {
        ESP_LOGE("DEPOSIT", "Max timers reached, cannot start rental for compartment %d", compartmentId);
        vPortExitCritical(&depositMutex);
        return;
    }

    RentalTimer timer;
    timer.compartmentId = compartmentId;
    timer.startMs = esp_timer_get_time() / 1000;
    timer.durationSec = durationSec;
    // Create FreeRTOS timer for overdue (duration + grace)
    vPortEnterCritical(&g_configMutex);
    unsigned long totalSec = durationSec + g_config.system.gracePeriodSec;
    vPortExitCritical(&g_configMutex);
    timer.timerHandle = xTimerCreate("OverdueTimer", pdMS_TO_TICKS(totalSec * 1000), pdFALSE, (void*)compartmentId, overdueCallback);
    if (timer.timerHandle == NULL) {
        ESP_LOGE("DEPOSIT", "Failed to create timer for compartment %d", compartmentId);
        vPortExitCritical(&depositMutex);
        return;
    }
    xTimerStart(timer.timerHandle, 0);
    activeTimers[activeTimersCount++] = timer;
    ESP_LOGI("DEPOSIT", "Started rental timer for compartment %d, duration %lu sec, overdue in %lu sec", compartmentId, durationSec, totalSec);
    vPortExitCritical(&depositMutex);
}

void checkOverdue() {
    // This can be called periodically as fallback, but timers handle it
    unsigned long now = esp_timer_get_time() / 1000;
    vPortEnterCritical(&depositMutex);
    for (int i = 0; i < activeTimersCount; ) {
        unsigned long elapsedSec = (now - activeTimers[i].startMs) / 1000;
        vPortEnterCritical(&g_configMutex);
        unsigned long totalAllowedSec = activeTimers[i].durationSec + g_config.system.gracePeriodSec;
        vPortExitCritical(&g_configMutex);
        if (elapsedSec > totalAllowedSec) {
            // Overdue: auto-lock (fallback if timer failed)
            ESP_LOGI("DEPOSIT", "Compartment %d overdue (fallback check), auto-locking", activeTimers[i].compartmentId);
            // TODO: Close gate
            xTimerDelete(activeTimers[i].timerHandle, 0);
            // Shift array
            for (int j = i; j < activeTimersCount - 1; j++) {
                activeTimers[j] = activeTimers[j + 1];
            }
            activeTimersCount--;
        } else {
            ++i;
        }
    }
    vPortExitCritical(&depositMutex);
}

void deposit_on_take(esp_mqtt_client_handle_t client) {
    vPortEnterCritical(&depositMutex);
    deposit_held = true;
    rental_start_time = esp_timer_get_time() / 1000;
    vPortEnterCritical(&g_configMutex);
    rental_duration_ms = g_config.system.gracePeriodSec * 1000;
    vPortExitCritical(&g_configMutex);
    vPortExitCritical(&depositMutex);
    ESP_LOGI("DEPOSIT", "Deposit held, rental started");
}

void deposit_on_return(esp_mqtt_client_handle_t client) {
    vPortEnterCritical(&depositMutex);
    if (!deposit_held) {
        vPortExitCritical(&depositMutex);
        return;
    }
    unsigned long elapsed = (esp_timer_get_time() / 1000) - rental_start_time;
    if (elapsed <= rental_duration_ms) {
        // On-time return: release deposit
        deposit_held = false;
        ESP_LOGI("DEPOSIT", "Deposit released (on-time return)");
        // Publish deposit release event
        cJSON *doc = cJSON_CreateObject();
        cJSON_AddStringToObject(doc, "action", "release");
        cJSON_AddNumberToObject(doc, "timestamp", esp_timer_get_time() / 1000);
        char *payload = cJSON_PrintUnformatted(doc);
        char topic[64];
        vPortEnterCritical(&g_configMutex);
        char locationCode[32];
        strcpy(locationCode, g_config.location.code);
        vPortExitCritical(&g_configMutex);
        snprintf(topic, sizeof(topic), "waterfront/%s/deposit/release", locationCode);
        esp_mqtt_client_publish(client, topic, payload, 0, 1, 0);
        cJSON_free(payload);
        cJSON_Delete(doc);
    } else {
        ESP_LOGW("DEPOSIT", "Late return, deposit not released");
    }
    vPortExitCritical(&depositMutex);
}

bool deposit_is_held() {
    vPortEnterCritical(&depositMutex);
    bool held = deposit_held;
    vPortExitCritical(&depositMutex);
    return held;
}
