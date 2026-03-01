# WATERFRONT ESP32 Kayak Rental Controller - System Specification

## Overview
The WATERFRONT system is an ESP32-based controller for automated kayak rental bays. It manages compartment gates, deposit logic, sensor monitoring, and remote connectivity via MQTT with TLS support. The system includes power management, OTA updates, and provisioning features.

## Hardware Requirements
- ESP32 microcontroller (e.g., ESP32-DevKitC)
- Ultrasonic sensors (HC-SR04) for kayak presence detection
- Servo motors for gate control
- Limit switches for gate position feedback
- LTE modem (optional, e.g., SIM7600) for cellular fallback
- Solar panel and battery for power
- ADC inputs for voltage monitoring

## Software Architecture
### Core Components
- **Main Loop**: Handles MQTT, OTA, LTE power management, and periodic power checks
- **MQTT Client**: Manages secure connections with TLS, client certs, and fallback
- **Config Loader**: Loads/saves JSON config from LittleFS with validation
- **Deposit Logic**: Manages rental timers, overdue detection, and auto-lock
- **Return Sensor**: Ultrasonic distance measurement for kayak presence
- **Gate Control**: Servo control with limit switch feedback
- **LTE Manager**: Cellular fallback with power management
- **Error Handler**: Fatal error logging and MQTT alerting
- **OTA Handler**: Over-the-air updates with NVS versioning
- **Provisioning**: BLE and SoftAP WiFi setup

### Tasks
- Factory Reset Task: Monitors GPIO 0 for long press
- Overdue Check Task: Periodic overdue rental checks
- Debug Task: Health telemetry publishing
- MQTT Loop Task: Maintains MQTT connection

## Configuration
Stored in `/config.json` on LittleFS. Includes MQTT, location, WiFi provisioning, LTE, BLE, compartments, system settings.

### Example Config
