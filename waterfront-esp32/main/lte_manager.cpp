#include "lte_manager.h"
#include "config_loader.h"
#include <esp_log.h>

// ESP-IDF modem instances
esp_modem_dce_t* dce = nullptr;
esp_netif_t* esp_netif = nullptr;
uart_port_t uart_num = UART_NUM_1;

// Mutex for thread-safe access to LTE state
portMUX_TYPE lteMutex = portMUX_INITIALIZER_UNLOCKED;

// Data usage tracking
static uint64_t lteDataUsageBytes = 0;  // Track data usage in bytes
static unsigned long lastDataCheck = 0;  // Last time data usage was checked

// Initialize LTE modem (power off initially)
void lte_init() {
    ESP_LOGI("LTE", "Initializing LTE modem");
    // Initialize UART for modem communication
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);  // TX, RX pins
    uart_driver_install(uart_num, 1024, 0, 0, NULL, 0);

    // Initialize modem DCE
    esp_modem_dce_config_t dce_config = ESP_MODEM_DCE_DEFAULT_CONFIG();
    dce = esp_modem_new(&dce_config, ESP_MODEM_DCE_MODE_COMMAND, uart_num);

    // Initialize netif
    esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_PPP();
    esp_netif = esp_netif_new(&netif_config);

    ESP_LOGI("LTE", "LTE modem initialized");
}

// Power up and configure the modem
void lte_power_up() {
    ESP_LOGI("LTE", "Powering up LTE modem");
    // Power on modem (assume PWRKEY pin control)
    // gpio_set_level(PWRKEY_PIN, 1);
    // vTaskDelay(pdMS_TO_TICKS(1000));
    // gpio_set_level(PWRKEY_PIN, 0);

    // Initialize PPP
    esp_modem_set_mode(dce, ESP_MODEM_MODE_DATA);
    esp_netif_attach(esp_netif, esp_modem_get_netif(dce));

    // Connect to GPRS
    vPortEnterCritical(&g_configMutex);
    const char* apn = g_config.lte.apn;
    vPortExitCritical(&g_configMutex);
    esp_modem_connect(dce, apn, "", "");

    ESP_LOGI("LTE", "LTE modem powered up and connected");
}

// Power down the modem to save energy
void lte_power_down() {
    ESP_LOGI("LTE", "Powering down LTE modem");
    esp_modem_disconnect(dce);
    // gpio_set_level(PWRKEY_PIN, 1);
    // vTaskDelay(pdMS_TO_TICKS(3000));
    // gpio_set_level(PWRKEY_PIN, 0);
    ESP_LOGI("LTE", "LTE modem powered down");
}

// Switch MQTT client to LTE and reconnect
void lte_switch_to_lte() {
    ESP_LOGI("LTE", "Switching to LTE");
    // Ensure LTE is powered up
    lte_power_up();
    // MQTT will use LTE netif
    ESP_LOGI("LTE", "Switched to LTE");
}

// Switch MQTT client back to WiFi and reconnect
void lte_switch_to_wifi() {
    ESP_LOGI("LTE", "Switching to WiFi");
    // Power down LTE
    lte_power_down();
    // MQTT will use WiFi netif
    ESP_LOGI("LTE", "Switched to WiFi");
}

// Get LTE signal quality
int lte_get_signal() {
    int rssi = 0;
    esp_modem_get_signal_quality(dce, &rssi);
    return rssi;
}

// Check if LTE is connected
bool lte_is_connected() {
    return esp_modem_is_connected(dce);
}

// Get current data usage in bytes
uint64_t lte_get_data_usage() {
    vPortEnterCritical(&lteMutex);
    uint64_t usage = lteDataUsageBytes;
    vPortExitCritical(&lteMutex);
    return usage;
}

// Reset data usage counter
void lte_reset_data_usage() {
    vPortEnterCritical(&lteMutex);
    lteDataUsageBytes = 0;
    vPortExitCritical(&lteMutex);
    ESP_LOGI("LTE", "Data usage reset");
}

// Update data usage (call periodically)
void lte_update_data_usage() {
    // Placeholder: In a real implementation, query modem for data usage
    // For now, estimate based on time connected
    unsigned long now = esp_timer_get_time() / 1000;
    if (lastDataCheck > 0) {
        unsigned long deltaTime = now - lastDataCheck;
        // Estimate 1 KB per second (adjust based on actual usage)
        vPortEnterCritical(&lteMutex);
        lteDataUsageBytes += deltaTime * 1024;
        vPortExitCritical(&lteMutex);
    }
    lastDataCheck = now;
}

// Check if LTE should be disabled (e.g., low solar or data limit exceeded)
bool shouldDisableLTE() {
    vPortEnterCritical(&g_configMutex);
    float minVoltage = g_config.system.solarVoltageMin;
    uint64_t maxUsage = g_config.lte.dataUsageAlertLimitKb * 1024ULL;
    vPortExitCritical(&g_configMutex);
    // Check solar voltage (placeholder: integrate with ADC)
    // For now, return false
    bool lowSolar = false;  // TODO: Implement solar check
    bool overUsage = lte_get_data_usage() > maxUsage;
    return lowSolar || overUsage;
}

// Power management for LTE based on conditions
void lte_power_management() {
    // Update data usage
    lte_update_data_usage();

    // Power down LTE if WiFi is connected and MQTT has been idle for >5 minutes
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK && mqttConnected && (esp_timer_get_time() / 1000 - lastMqttActivity > 300000)) {  // 5 min idle
        lte_power_down();
        ESP_LOGI("LTE", "Powered down due to WiFi connected and idle > 5 min");
    }
    // Power down LTE if solar voltage is low to conserve power
    if (shouldDisableLTE()) {
        lte_power_down();
        ESP_LOGI("LTE", "Powered down due to low solar or data limit exceeded");
    }
    // Log signal quality periodically
    static unsigned long lastSignalCheck = 0;
    if (esp_timer_get_time() / 1000 - lastSignalCheck > 60000) {  // Every minute
        int rssi = lte_get_signal();
        ESP_LOGI("LTE", "Signal quality: %d dBm", rssi);
        lastSignalCheck = esp_timer_get_time() / 1000;
    }
}
