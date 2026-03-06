#ifndef LTE_MANAGER_H
#define LTE_MANAGER_H

#include <esp_modem.h>        // ESP-IDF modem for cellular
#include <driver/uart.h>      // ESP-IDF UART for modem communication

// External ESP-IDF modem instances for global access
extern esp_modem_dce_t* dce;  // Modem DCE object
extern esp_netif_t* esp_netif; // Netif for LTE
extern uart_port_t uart_num; // UART port for modem

// Initialize LTE modem (power off initially)
void lte_init();

// Power up and configure the modem
void lte_power_up();

// Power down the modem to save energy
void lte_power_down();

// Switch MQTT client to LTE and reconnect
void lte_switch_to_lte();

// Switch MQTT client back to WiFi and reconnect
void lte_switch_to_wifi();

// Get LTE signal quality
int lte_get_signal();

// Check if LTE is connected
bool lte_is_connected();

// Get current data usage in bytes
uint64_t lte_get_data_usage();

// Reset data usage counter
void lte_reset_data_usage();

// Update data usage (call periodically)
void lte_update_data_usage();

// Check if LTE should be disabled (e.g., low solar)
bool shouldDisableLTE();

// Power management for LTE based on conditions
void lte_power_management();

#endif // LTE_MANAGER_H
