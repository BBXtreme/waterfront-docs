# WATERFRONT ESP32 Kayak Rental Controller - System Specification

## What is This System?
The WATERFRONT system is a smart device for kayak rental stations. It automatically manages gates for kayak storage compartments. Built on an ESP32 microcontroller, it connects to the internet to receive rental bookings and send status updates. It's made for outdoor use with solar power and cellular backup.

## Why Use It?
- **Automation**: No need for manual gate opening/closing.
- **Remote Monitoring**: Check status from anywhere via internet.
- **Reliability**: Handles power outages and network issues.
- **Scalability**: Supports multiple compartments.

## Hardware Components
- **ESP32 Microcontroller**: The brain, handles logic and communication.
- **Ultrasonic Sensors**: Measure distance to detect kayaks (sound waves bounce back).
- **Servo Motors**: Mechanical arms that open/close gates.
- **Limit Switches**: Sensors that detect if gates are fully open or closed.
- **LTE Modem** (optional): Cellular module for internet without WiFi.
- **Solar Panel and Battery**: Powers the system sustainably.
- **ADC Inputs**: Measure voltage levels for power monitoring.

## Software Architecture
The code is organized into modules, each handling a specific job.

### Main Modules
- **Main Loop**: Runs forever, checks power, handles MQTT, manages tasks.
- **MQTT Handler**: Sends/receives messages securely.
- **Config Loader**: Reads settings from a file.
- **Deposit Logic**: Manages rental payments and timers.
- **Return Sensor**: Detects kayak presence.
- **Gate Control**: Moves servos, checks switches.
- **LTE Manager**: Switches to cellular if WiFi fails.
- **Error Handler**: Logs errors, sends alerts.
- **OTA Updater**: Downloads new software wirelessly.
- **Provisioning**: Sets up WiFi easily.

### Background Tasks
- **Factory Reset Task**: Listens for button press to reset device.
- **Overdue Check Task**: Closes gates for late returns.
- **Debug Task**: Sends health updates every minute.

## Configuration Details
All settings are in `/config.json` on the ESP32's storage. This file is in JSON format (easy to read/edit).

### Sections Explained
- **mqtt**: Internet connection settings (server, login, security).
- **location**: Your location name and code.
- **wifiProvisioning**: Default WiFi for setup.
- **lte**: Cellular settings if using modem.
- **ble**: Bluetooth settings for setup.
- **compartments**: List of gates, with pin numbers for hardware.
- **system**: General settings like debug mode and power thresholds.
- **other**: Extra options like offline PIN time.

### Example Config
