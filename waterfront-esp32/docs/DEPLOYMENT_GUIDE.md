# WATERFRONT ESP32 Deployment Guide

## Prerequisites
- ESP32 development board
- PlatformIO IDE
- MQTT broker with TLS support
- CA certificate for TLS
- Optional: LTE modem, sensors, servos

## Installation
1. Clone repository
2. Install dependencies: `pio lib install`
3. Configure platformio.ini for your board
4. Build and upload: `pio run --target upload`

## Configuration
1. Create config.json with your settings
2. Upload config.json to LittleFS: `pio run --target uploadfs`
3. Upload certs (ca.pem, etc.) to LittleFS

## Provisioning
1. Power on ESP32
2. Use BLE app (e.g., nRF Connect) to connect and send WiFi credentials
3. Or connect to SoftAP and use web interface
4. Device connects to WiFi and MQTT

## Monitoring
- Subscribe to MQTT debug topic for telemetry
- Check serial logs for issues
- Use OTA for updates

## Maintenance
- Regularly check battery and solar levels
- Update firmware via OTA
- Monitor error logs

## Troubleshooting
- If no WiFi, use SoftAP provisioning
- If MQTT fails, check certs and broker settings
- Factory reset via GPIO 0 long press
