# ToDo ESP32 Firmware

Date: 01.03.2026

**Security** (currently ~40 %)

- MQTT is plain TCP (no TLS/MQTTS on 8883)
- No username/password authentication enforced in code (connect() without credentials)
- No payload signing / HMAC / simple checksum on critical commands (unlock, config update)

**Reliability / Hardening** (~65 %)

- No ESP task watchdog (esp_task_wdt) → single infinite loop can hang the device forever
- No comprehensive error recovery (e.g. repeated MQTT reconnect backoff, NVS fallback for config corruption)
- No CRC or any integrity check on config.json after remote update
- Deep sleep / power management still partial (not fully wired to solar voltage monitoring)

**Testing / CI** (~45 %)

- Only mqtt_handler has tests; gate_control, compartment_manager, return_sensor, lte_manager have zero
- No GitHub Actions workflow that runs pio run + pio test on push
- No hardware-in-the-loop or simulated sensor tests

**Documentation inside waterfront-esp32** (~70 %)

- No dedicated README.md inside waterfront-esp32
- config.h is deprecated but still exists with comments — confusing
- No quick "how to flash + test locally" guide specific to ESP32

**Production Readiness** (~60 %)

- OTA update mechanism missing
- No remote firmware version reporting / OTA trigger via MQTT
- No factory reset button logic (long-press → erase NVS + config.json)
- SIM7600 LTE power gating / SIM PIN unlock not fully proven