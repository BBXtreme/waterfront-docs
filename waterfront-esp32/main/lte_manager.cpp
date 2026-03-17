#include "lte_manager.h"
#include <esp_timer.h>          // ← added

static const char* TAG = "LTE";
static uint64_t lteDataUsageBytes = 0;
static unsigned long lastDataCheck = 0;
static bool lteActive = false;

void lte_init(void) {
    ESP_LOGW(TAG, "LTE stub initialized (mockup mode)");
}

void lte_power_up(void) { lteActive = true; ESP_LOGI(TAG, "LTE powered up"); }
void lte_power_down(void) { lteActive = false; ESP_LOGI(TAG, "LTE powered down"); }
void lte_switch_to_lte(void) { lte_power_up(); }
void lte_switch_to_wifi(void) { lte_power_down(); }
int lte_get_signal(void) { return -70; }
bool lte_is_connected(void) { return lteActive; }
uint64_t lte_get_data_usage(void) { return lteDataUsageBytes; }
void lte_reset_data_usage(void) { lteDataUsageBytes = 0; }

void lte_update_data_usage(void) {
    unsigned long now = esp_timer_get_time() / 1000;
    if (now - lastDataCheck > 5000) {
        if (lteActive) lteDataUsageBytes += 1024;
        lastDataCheck = now;
    }
}

bool shouldDisableLTE(void) {
    vPortEnterCritical(&g_configMutex);
    float minVoltage = g_config.system.solarVoltageMin;
    uint64_t maxUsage = g_config.lte.dataUsageAlertLimitKb * 1024ULL;
    vPortExitCritical(&g_configMutex);

    bool lowSolar = false;
    bool overUsage = lte_get_data_usage() > maxUsage;
    return lowSolar || overUsage;
}

void lte_power_management(void) {
    lte_update_data_usage();
    if (shouldDisableLTE()) {
        lte_power_down();
        ESP_LOGI(TAG, "LTE powered down due to low solar or data limit");
    }
}