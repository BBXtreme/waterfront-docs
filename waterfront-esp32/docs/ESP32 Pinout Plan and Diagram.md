# ESP32 Pinout Plan and Diagram  
**WATERFRONT Kayak Rental Controller Firmware**

**Version**: 1.2  
**Date**: February 28, 2026  
**Author**: Grok (xAI) – assisted development for BangLee  
**Purpose**: Define safe, conflict-free GPIO assignments for the ESP32-based unmanned kayak vending controller.  
**Targets**: ESP32 classic (DevKitC V4 / WROOM-32) and ESP32-S3 (DevKitC-1 recommended for PSRAM and future expansion).  
**Related Documents**: TSD §4 (ESP32 Firmware Extensions), FSD FR-ESP-01–04, Project Plan Task 1.5 & 2.5.

This document provides recommended GPIO pin assignments tailored to WATERFRONT requirements:  
MQTT-based unlock commands, sensor-driven return confirmation, relay-controlled compartment lock, LTE cellular failover, and runtime WiFi provisioning (BLE/SoftAP).

**Important**: All GPIOs listed are safe for normal operation (no conflict with boot, flash SPI, or WiFi/BT radio after initialization). Final pin selection depends on your exact board model and peripherals.

## 1. Recommended GPIO Assignments

| Function                                        | Classic ESP32 GPIO | ESP32-S3 GPIO | Direction | Notes / Requirements                                         |
| ----------------------------------------------- | ------------------ | ------------- | --------- | ------------------------------------------------------------ |
| Relay / Solenoid Lock (compartment open)        | 23                 | 21            | Output    | Active-high pulse (1–2 s recommended). Use opto-isolated relay module for 12 V solenoid. |
| Ultrasonic Sensor Trigger (HC-SR04 or similar)  | 5                  | 4             | Output    | Safe digital output for trigger pulse.                       |
| Ultrasonic Sensor Echo                          | 18                 | 5             | Input     | Standard HC-SR04 (5 V) requires voltage divider (1 kΩ + 2 kΩ) to 3.3 V. HC-SR04+ works directly. |
| LTE Modem UART TX (ESP → modem RX)              | 17                 | 17            | Output    | Hardware UART2 (UART_NUM_2). Avoid UART0 (used for Serial debug/programming). |
| LTE Modem UART RX (modem TX → ESP)              | 16                 | 18            | Input     | Same UART2. Optional PWRKEY control on GPIO 4.               |
| Provisioning Button (BLE/SoftAP WiFi config)    | 4 or 0             | 0 or 45       | Input     | Internal pull-up enabled. Software debounce required. GPIO 0 can reuse native BOOT button. |
| Status LED (heartbeat / MQTT connected / error) | 2 or 27            | 48 or onboard | Output    | GPIO 2 is onboard LED on many classic DevKits. S3 boards often use RGB GPIO 48. |
| Battery Voltage Monitoring (ADC)                | 34 (ADC1_CH6)      | 1–20 (ADC1)   | Input     | Input-only pin on classic ESP32. Use voltage divider for batteries > 3.3 V. |
| Deep Sleep Wake Source (external interrupt)     | 32–39 range        | RTC GPIOs     | Input     | RTC-capable pins only. Suggested: GPIO 33 for sensor-triggered wake on return. |

### Key Safety & Design Rules
- **Boot/Strapping pins to avoid**: GPIO 0, 2, 12, 15 (affect boot mode or flash)
- **Flash SPI pins (never use)**: GPIO 6–11
- **ADC2 restriction**: ADC2 channels are disabled during WiFi/BT operation → prefer ADC1 pins
- **Power considerations**: LTE modem typically requires separate 3.8–4.2 V supply with high current capability. Always use common ground between ESP32, modem, relay, and sensors.
- **Future-proofing**: Reserve GPIO 21/22 (I2C) or 19/22 for optional OLED display, temperature sensor, or multi-bay expansion.

## 2. Pinout Diagrams – Visual References

### ESP32 DevKitC V4 (Classic ESP32-WROOM-32)

**Typical header layout** (left side top-to-bottom, right side mirrored):  
Left: 3V3, EN, 36(VP), 39(VN), 34, 35, 32, 33, 25, 26, 27, 14, 12, 13, GND, 5V  
Right: GND, 23, 22, 21, 19, 18, 17, 16, 4, 0, 2, 15, 13, 12, GND, TX0, RX0

**WATERFRONT recommended mapping**:
- Relay → 23
- Ultrasonic Trig → 5
- Ultrasonic Echo → 18
- LTE UART TX → 17
- LTE UART RX → 16
- Provisioning Button → 4
- Status LED → 2 (often onboard)

### ESP32-S3 DevKitC-1 (Recommended for PSRAM)

Modern pin layout (varies slightly by revision – v1.0/v1.1).  
Safe output GPIOs: mostly 1–21 and 35–48. ADC channels differ from classic ESP32.

**WATERFRONT recommended mapping**:
- Relay → 21
- Ultrasonic Trig → 4
- Ultrasonic Echo → 5
- LTE UART TX → 17
- LTE UART RX → 18
- Provisioning Button → 0 (or 45/46)
- Status LED → 48 (or onboard RGB)

## 3. Firmware Integration – config.h Example

Use these `#define` macros in `main/config.h` to make pins configurable:

```cpp
// main/config.h
#pragma once

// === Pin Definitions – WATERFRONT Kayak Rental Controller ===

#if CONFIG_IDF_TARGET_ESP32
  #define RELAY_PIN               23
  #define ULTRASONIC_TRIG_PIN     5
  #define ULTRASONIC_ECHO_PIN     18
  #define LTE_UART_TX_PIN         17
  #define LTE_UART_RX_PIN         16
  #define PROVISIONING_BUTTON_PIN 4
  #define STATUS_LED_PIN          2
  #define BATTERY_ADC_PIN         34
#elif CONFIG_IDF_TARGET_ESP32S3
  #define RELAY_PIN               21
  #define ULTRASONIC_TRIG_PIN     4
  #define ULTRASONIC_ECHO_PIN     5
  #define LTE_UART_TX_PIN         17
  #define LTE_UART_RX_PIN         18
  #define PROVISIONING_BUTTON_PIN 0
  #define STATUS_LED_PIN          48
  #define BATTERY_ADC_PIN         1     // adjust to valid ADC1 pin on S3
#else
  #error "Unsupported target – only ESP32 and ESP32-S3 are currently supported"
#endif

// UART configuration for LTE modem (always UART2)
#define LTE_UART_NUM            UART_NUM_2
#define LTE_UART_BAUD           115200

// Timing and behavior constants
#define RELAY_PULSE_DURATION_MS 1500    // unlock pulse length
#define SENSOR_POLL_INTERVAL_MS 10000   // poll every 10 seconds in normal mode
#define DEEP_SLEEP_WAKE_GPIO    33      // example RTC pin for sensor interrupt wake
```

## 4. Next Actions – Required Confirmation

To finalize this document and proceed with code generation:

1. **Board model** — Please provide exact model (e.g. "ESP32-DevKitC V4", "ESP32-S3-DevKitC-1-N8R2") with photo/link/markings if possible.
2. **Sensor type** — Ultrasonic (HC-SR04?), magnetic reed switch, load cell + HX711, IR barrier? This may affect pin choice or additional components.
3. **Lock hardware** — 12 V solenoid + relay module? Electronic cabinet lock? Power source details?
4. **LTE modem** — Model (e.g. SIM7600G-H, SIM7000G)? Logic level (1.8 V / 3.3 V)? Will PWRKEY / RESET pins be controlled by ESP32?

Once these are confirmed, we can:

- Lock in final GPIOs
- Update this document with your exact mapping
- Generate initial code skeletons (main.cpp, relay_handler.cpp, etc.) using these pins

## 5. Provisioning Integration Notes (Added for Runtime WiFi Config)

- **BLE Provisioning**: Uses GPIO 0/4 for button trigger. No additional pins needed; BT radio handles BLE.
- **SoftAP Fallback**: Temporary AP mode; no extra pins, but LED (GPIO 2) can blink to indicate password (e.g., number of blinks = password length).
- **NVS Storage**: Credentials saved in "wifi" namespace; no pin conflicts.
- **Power Impact**: Provisioning is short-lived; deep sleep unaffected.
- **Testing**: See updated test checklist in docs/ESP32 Firmware Test Checklist.md for nRF Connect steps.Ok. The code has been reviewed and debugged. Main issues were ESP_LOGI calls in Arduino framework, which have been replaced with Serial.println. The code should now compile in PlatformIO with Arduino framework. Test by flashing to ESP32 and checking Serial output for provisioning and MQTT connection. If issues persist, provide Serial logs for further debugging.