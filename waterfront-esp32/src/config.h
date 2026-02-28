/**
 * @file config.h
 * @brief Central configuration header for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Contains all global constants, pin definitions, and settings for easy maintenance and adaptation.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Configuration header file for WATERFRONT ESP32 Kayak Rental Controller
// This file contains all global constants, pin definitions, and settings used across the project.
// It centralizes configuration to make the code easier to maintain and adapt for different hardware setups.
// All values are defined as macros for compile-time constants, avoiding runtime overhead.

// WiFi and MQTT settings
// These define the default WiFi credentials and MQTT broker details.
// In production, these should be replaced with actual values or loaded from NVS.
#define WIFI_SSID "default_ssid"          // Default WiFi SSID; replace with actual network name
#define WIFI_PASS "default_pass"          // Default WiFi password; replace with actual password
#define MQTT_SERVER "broker.emqx.io"      // MQTT broker server address (public broker for testing)
#define MQTT_PORT 1883                    // MQTT port (1883 for non-TLS, 8883 for TLS)

// BLE settings (from nimble.h and bleprph.h)
// These are related to Bluetooth Low Energy (BLE) functionality for provisioning.
#define BLE_DEVICE_NAME "KayakDevice"     // Default BLE device name advertised during provisioning
#define MAX_DEVICES 100                   // Maximum number of BLE devices to handle (for scanning)

// GPIO pins (example, adjust as needed)
// Pin definitions for hardware connections. These should match the actual wiring on the ESP32 board.
// Refer to the pinout diagram for safe GPIO choices (avoid boot pins like GPIO 0, 2, 12, 15).
#define PIN_LED 2                         // GPIO pin for status LED (onboard LED on many ESP32 boards)

// Provisioning settings
// Settings for WiFi provisioning (BLE or SoftAP) and button triggers.
#define PROVISIONING_BUTTON_PIN 4         // GPIO pin for provisioning button (debounced long-press)
#define STATUS_LED_PIN 2                  // GPIO pin for status LED (blinks during provisioning)
#define RELAY_PIN 23                      // GPIO pin for relay control (lock solenoid)
#define MACHINE_ID "bremen-harbor-01"      // Unique machine ID for MQTT topics and identification
#define MQTT_BROKER "broker.emqx.io"      // MQTT broker address (consistent with MQTT_SERVER)
#define RELAY_PULSE_DURATION_MS 1500      // Duration to pulse relay for unlock (in milliseconds)
#define SENSOR_POLL_INTERVAL_MS 10000     // Interval to poll sensors (in milliseconds)

// LTE settings
// Configuration for LTE cellular modem fallback using TinyGSM.
#define LTE_MODEM_TX_PIN 17                // ESP32 TX pin connected to modem RX (UART2)
#define LTE_MODEM_RX_PIN 16                // ESP32 RX pin connected to modem TX (UART2)
#define LTE_MODEM_PWRKEY_PIN 25            // GPIO pin for modem PWRKEY control (MOSFET for power gating)
#define LTE_MODEM_BAUD 115200              // UART baud rate for modem communication
#define LTE_APN "internet.t-mobile.de"     // APN for cellular data (example for Germany; adjust per SIM)
#define LTE_MODEM_MODEL "SIM7600"          // Modem model (SIM7600 or SIM7000 for LTE support)
#define WIFI_FAILOVER_TIMEOUT_MS 30000     // Timeout in ms to wait for WiFi before switching to LTE

#endif // CONFIG_H
