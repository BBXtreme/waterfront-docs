# Waterfront ESP32 Firmware

ESP32 firmware for Waterfront unmanned kayak rental compartments. Controls servo gates, detects presence with ultrasonic sensors, handles MQTT-based unlock commands, and supports runtime configuration updates.

## Project Overview

This firmware runs on ESP32-S3 (or ESP32 classic) to manage unmanned rental compartments for Waterfront. It connects to an MQTT broker for real-time control, publishes telemetry, and supports WiFi provisioning + LTE failover. All settings are loaded from `/config.json` on LittleFS, enabling remote updates without reflashing.

Key features:

- MQTT-based unlock on payment (Stripe/BTCPay webhooks)
- Sensor-driven return confirmation (auto-lock)
- Overdue timer with auto-lock
- BLE/SoftAP WiFi provisioning
- LTE cellular fallback
- Watchdog for stability
- Unit tests with Catch2

## Hardware Requirements

- **ESP32-S3 DevKitC-1-N8R2** (recommended; or ESP32 classic)
- **SIM7600G-H LTE modem** (UART on GPIO16/17, PWRKEY on GPIO25)
- **Servo motor** (e.g., SG90, for gate control)
- **Ultrasonic sensor** (HC-SR04 or HC-SR04+, trigger/echo pins)
- **Limit switches** (2 per compartment: open/close detection)
- **Power supply** (3V ESP32 + 12V for solenoid, solar optional)
- **Pins loaded from config.json** (no hard-codes; see Configuration section)

Example wiring (pins from config.json):

- Servo: GPIO12
- Ultrasonic Trig: GPIO5, Echo: GPIO18
- LTE TX: GPIO17, RX: GPIO16
- Limit Open: GPIO13, Close: GPIO14

## Quick Start

1. **Install PlatformIO** (VS Code extension or CLI).

2. **Clone repo** and navigate to `waterfront-esp32/`.

3. **Build and upload firmware**:

   ```bash
   pio run -t upload
   ```

4. **Upload filesystem** (includes initial `/config.json`):

   ```bash
   pio run -t uploadfs
   ```

5. **Monitor serial output**:

   ```bash
   pio device monitor -b 115200
   ```

   Expected: "WATERFRONT starting..." and MQTT connection logs.

6. **Test MQTT unlock** (from broker):

   ```bash
   mosquitto_pub -h localhost -t "waterfront/bremen/harbor-01/compartments/1/command" -m "open_gate"
   ```

   Gate should open, and ESP32 publishes ack.

## Configuration

All settings are in `data/config.json` (uploaded to LittleFS). No hard-codes in code — everything loads at runtime.

- **Remote update**: Publish new JSON to `waterfront/{location}/{locationCode}/config/update` topic. ESP32 validates, saves, and restarts.

- **Example config.json**:

  ```json
  {
    "mqtt": {
      "broker": "192.168.178.50",
      "port": 8883,
      "username": "mqttuser",
      "password": "strongpass123",
      "clientIdPrefix": "waterfront",
      "useTLS": true,
      "caCertPath": "/ca.pem"
    },
    "location": {
      "slug": "bremen",
      "code": "harbor-01"
    },
    "wifiProvisioning": {
      "fallbackSsid": "WATERFRONT-DEFAULT",
      "fallbackPass": "defaultpass123"
    },
    "lte": {
      "apn": "internet.t-mobile.de",
      "simPin": "",
      "rssiThreshold": -70,
      "dataUsageAlertLimitKb": 100000
    },
    "ble": {
      "serviceUuid": "12345678-1234-1234-1234-123456789abc",
      "ssidCharUuid": "87654321-4321-4321-4321-cba987654321",
      "passCharUuid": "87654321-4321-4321-4321-dba987654321",
      "statusCharUuid": "87654321-4321-4321-4321-eba987654321"
    },
    "compartments": [
      {
        "number": 1,
        "servoPin": 12,
        "limitOpenPin": 13,
        "limitClosePin": 14,
        "ultrasonicTriggerPin": 15,
        "ultrasonicEchoPin": 16,
        "weightSensorPin": 17
      }
    ],
    "system": {
      "maxCompartments": 10,
      "debugMode": true,
      "gracePeriodSec": 3600,
      "batteryLowThresholdPercent": 20,
      "solarVoltageMin": 3.0
    },
    "other": {
      "offlinePinTtlSec": 86400,
      "depositHoldAmountFallback": 50
    }
  }
  ```

## Testing

- **Unit tests** (Catch2, mocks hardware):

  ```bash
  pio test -e unit_test
  ```

  Covers MQTT callbacks, gate control, compartment loading.

- **Manual MQTT tests** (with Mosquitto broker):

  - Status publish: `mosquitto_pub -h localhost -t "waterfront/bremen/harbor-01/compartments/1/status" -m '{"booked":true,"gateState":"locked"}'`
  - Command: `mosquitto_pub -h localhost -t "waterfront/bremen/harbor-01/compartments/1/command" -m "open_gate"`
  - Config update: `mosquitto_pub -h localhost -t "waterfront/bremen/harbor-01/config/update" -m '{"mqtt":{"broker":"new.ip"}}'`

## Debug Tips

- **Serial monitor**: 115200 baud. Logs ESP_LOGI/W/E for connection, unlocks, errors.
- **ESP_LOG levels**: Set `debugMode: true` in config for verbose output (e.g., sensor distances, MQTT payloads).
- **Common issues**:
  - Wrong broker: Check logs for "MQTT Failed to connect". Verify IP/port in config.
  - Pin conflicts: Use config.json to assign unique GPIOs. Avoid boot pins (0,2,12,15).
  - No unlock: Ensure MQTT topic matches location/code in config. Check broker auth.
  - LTE fallback: If WiFi fails, ESP32 auto-switches (logs "LTE MQTT switched to LTE").
- **Watchdog resets**: If ESP32 hangs, it auto-restarts (15s timeout). Check logs for fatal errors.

## Development Notes

- **Non-blocking design**: All loops use `millis()` + FreeRTOS tasks (e.g., overdue check every 10s). No `delay()`.
- **Dynamic compartments**: Loaded from config at boot; supports 1–10 compartments per device.
- **Security TODO**: MQTT TLS + username/password enforced (broker-side). Client certs optional.
- **Power optimization**: Deep sleep on idle; modem power gating for LTE.
- **Code style**: Arduino-compatible, modular (one .cpp/.h per feature), with extern g_config.

## Folder Structure

```
waterfront-esp32/
├── src/                    # Source files (.cpp/.h)
│   ├── main.cpp            # Entry point, setup/loop, tasks
│   ├── config_loader.cpp   # Load/save config.json from LittleFS
│   ├── mqtt_client.cpp     # MQTT connect/publish/subscribe
│   ├── gate_control.cpp    # Servo + limit switch logic
│   ├── return_sensor.cpp   # Ultrasonic presence detection
│   ├── deposit_logic.cpp   # Rental timers, overdue auto-lock
│   ├── wifi_manager.cpp    # WiFi station mode
│   ├── nimble.cpp          # BLE provisioning
│   ├── webui_server.cpp   # SoftAP provisioning
│   ├── lte_manager.cpp     # LTE modem control
│   ├── offline_fallback.cpp # PIN validation without MQTT
│   └── error_handler.cpp   # Fatal error logging + restart
├── test/                   # Unit tests (Catch2)
│   ├── test_mqtt_handler.cpp
│   ├── test_gate_control.cpp
│   └── test_compartment_manager.cpp
├── data/                   # LittleFS files (uploaded via pio run -t uploadfs)
│   └── config.json         # Runtime configuration
├── platformio.ini          # PlatformIO config (ESP32-S3, libs, build flags)
└── README.md               # This file
```

For issues or contributions, open a GitHub issue. Happy coding!
