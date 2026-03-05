#include "deposit_logic.h"
#include "config_loader.h"
#include <PubSubClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

std::vector<RentalTimer> activeTimers;
bool deposit_held = false;
unsigned long rental_start_time = 0;
unsigned long rental_duration_ms = 0;

// Overdue callback for FreeRTOS timer
void overdueCallback(TimerHandle_t xTimer) {
    int compartmentId = (int)pvTimerGetTimerID(xTimer);
    ESP_LOGI("DEPOSIT", "Compartment %d overdue, auto-locking", compartmentId);
    // Auto-lock: close the gate (assuming gate_control is available)
    // TODO: Integrate with gate_control.h to close the gate
    // For now, log and remove from activeTimers
    for (auto it = activeTimers.begin(); it != activeTimers.end(); ) {
        if (it->compartmentId == compartmentId) {
            xTimerDelete(it->timerHandle, 0);
            it = activeTimers.erase(it);
            break;
        } else {
            ++it;
        }
    }
    // Publish overdue event if MQTT available
    // TODO: Publish MQTT message for overdue
}

void deposit_init() {
    deposit_held = false;
    ESP_LOGI("DEPOSIT", "Initialized");
}

void startRental(int compartmentId, unsigned long durationSec) {
    // Check if already active
    for (const auto& timer : activeTimers) {
        if (timer.compartmentId == compartmentId) {
            ESP_LOGW("DEPOSIT", "Rental already active for compartment %d", compartmentId);
            return;
        }
    }

    RentalTimer timer;
    timer.compartmentId = compartmentId;
    timer.startMs = millis();
    timer.durationSec = durationSec;
    // Create FreeRTOS timer for overdue (duration + grace)
    vPortEnterCritical(&g_configMutex);
    unsigned long totalSec = durationSec + g_config.system.gracePeriodSec;
    vPortExitCritical(&g_configMutex);
    timer.timerHandle = xTimerCreate("OverdueTimer", pdMS_TO_TICKS(totalSec * 1000), pdFALSE, (void*)compartmentId, overdueCallback);
    if (timer.timerHandle == NULL) {
        ESP_LOGE("DEPOSIT", "Failed to create timer for compartment %d", compartmentId);
        return;
    }
    xTimerStart(timer.timerHandle, 0);
    activeTimers.push_back(timer);
    ESP_LOGI("DEPOSIT", "Started rental timer for compartment %d, duration %lu sec, overdue in %lu sec", compartmentId, durationSec, totalSec);
}

void checkOverdue() {
    // This can be called periodically as fallback, but timers handle it
    unsigned long now = millis();
    for (auto it = activeTimers.begin(); it != activeTimers.end(); ) {
        unsigned long elapsedSec = (now - it->startMs) / 1000;
        vPortEnterCritical(&g_configMutex);
        unsigned long totalAllowedSec = it->durationSec + g_config.system.gracePeriodSec;
        vPortExitCritical(&g_configMutex);
        if (elapsedSec > totalAllowedSec) {
            // Overdue: auto-lock (fallback if timer failed)
            ESP_LOGI("DEPOSIT", "Compartment %d overdue (fallback check), auto-locking", it->compartmentId);
            // TODO: Close gate
            xTimerDelete(it->timerHandle, 0);
            it = activeTimers.erase(it);
        } else {
            ++it;
        }
    }
}

void deposit_on_take(PubSubClient* client) {
    deposit_held = true;
    rental_start_time = millis();
    vPortEnterCritical(&g_configMutex);
    rental_duration_ms = g_config.system.gracePeriodSec * 1000;
    vPortExitCritical(&g_configMutex);
    ESP_LOGI("DEPOSIT", "Deposit held, rental started");
}

void deposit_on_return(PubSubClient* client) {
    if (!deposit_held) return;
    unsigned long elapsed = millis() - rental_start_time;
    if (elapsed <= rental_duration_ms) {
        // On-time return: release deposit
        deposit_held = false;
        ESP_LOGI("DEPOSIT", "Deposit released (on-time return)");
        // Publish deposit release event
        char topic[64];
        char payload[128];
        vPortEnterCritical(&g_configMutex);
        String locationCode = g_config.location.code;
        vPortExitCritical(&g_configMutex);
        snprintf(topic, sizeof(topic), "waterfront/%s/deposit/release", locationCode.c_str());
        snprintf(payload, sizeof(payload), "{\"action\":\"release\",\"timestamp\":%lu}", millis());
        client->publish(topic, payload);
    } else {
        ESP_LOGW("DEPOSIT", "Late return, deposit not released");
    }
}

bool deposit_is_held() {
    return deposit_held;
}
