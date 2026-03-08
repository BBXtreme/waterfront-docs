#ifndef LTE_MANAGER_H
#define LTE_MANAGER_H

#include "esp_log.h"
#include "config_loader.h"

// Stub functions for mockup phase (no real modem yet)
void lte_init(void);
void lte_power_up(void);
void lte_power_down(void);
void lte_switch_to_lte(void);
void lte_switch_to_wifi(void);
int lte_get_signal(void);
bool lte_is_connected(void);
uint64_t lte_get_data_usage(void);
void lte_reset_data_usage(void);
void lte_update_data_usage(void);
bool shouldDisableLTE(void);
void lte_power_management(void);

#endif // LTE_MANAGER_H