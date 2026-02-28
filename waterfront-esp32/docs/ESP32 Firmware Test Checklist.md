### 3. Prepare a Minimal Test List / Acceptance Criteria (30–60 min)

**Purpose**: simple pass/fail criteria for the first milestones. 



Example:

#### WATERFRONT ESP32 Firmware – Test Checklist

###### Phase 1 – Basic Connectivity

- [ ] Board boots and prints "WATERFRONT starting..." via Serial
- [ ] Connects to WiFi (provisioned or hardcoded)
- [ ] Connects to MQTT broker and subscribes to /kayak/{machineId}/unlock
- [ ] Publishes status message every 60 s

###### Phase 2 – Lock & Sensor

- [ ] Relay pulses HIGH for 1.5 s on valid unlock command
- [ ] Sensor detects "taken" → publishes event → auto-locks after timeout
- [ ] Sensor detects "returned" → publishes event

###### Phase 3 – Failover & Offline

- [ ] Disconnect WiFi → switches to LTE within 30 s
- [ ] Offline mode: accepts pre-synced PIN via button press (future)

###### Phase 4 – WiFi Provisioning (New)

- [ ] On first boot (no NVS credentials), starts BLE provisioning
- [ ] Use nRF Connect app: Connect to ESP32 device, write SSID/password to GATT characteristic
- [ ] ESP32 connects to WiFi, publishes {"wifiState":"connected","ssid":"...","ip":"..."} via MQTT
- [ ] Long-press button (3s) triggers provisioning (even if already connected)
- [ ] If BLE fails, falls back to SoftAP: SSID "WATERFRONT-PROV", random password shown on LED
- [ ] Connect phone to SoftAP, open captive portal (192.168.4.1), enter WiFi creds
- [ ] After success, stops provisioning, restarts STA, reconnects MQTT
- [ ] Future: MQTT command /kayak/{machineId}/provision/start triggers provisioning (admin only)
- [ ] Credentials stored in NVS namespace "wifi"
- [ ] Keep existing MQTT client alive.

**Test Steps for Provisioning (Beginner Guide)**:
1. Flash firmware to ESP32.
2. On first boot, Serial should show "WiFi not provisioned, starting provisioning..."
3. For BLE: Install nRF Connect app on phone. Scan for "PROV_XXXXXX" device. Connect. Find GATT service (UUID: 0x180F or provisioning service). Write SSID to one char, password to another. ESP32 should connect.
4. For SoftAP: If BLE not working, it auto-falls back. Connect phone WiFi to "WATERFRONT-PROV" with printed password. Browser opens captive portal automatically. Enter home WiFi SSID/password. Submit.
5. Check Serial for "Provisioning successful" and MQTT publish.
6. Reboot ESP32: Should auto-connect without provisioning.
7. Test button: Hold GPIO 4 low for 3s, Serial shows "Provisioning button long-pressed...". Repeats provisioning.

**Debug Tips**:
- If BLE doesn't advertise: Check NimBLE-Arduino version, ensure no BT conflicts.
- SoftAP password: Printed in Serial; for hardware, add LED blink pattern (e.g., blink count = password length).
- NVS issues: Erase flash if stuck (idf.py erase-flash).
- MQTT reconnect: Ensure broker is reachable after WiFi change.

**Test Steps for LTE Failover (New)**:
1. Flash firmware with LTE hardware connected (SIM7600 on UART2, PWRKEY on GPIO25).
2. Insert SIM card with data plan (APN: internet.t-mobile.de for DE).
3. Boot ESP32: Should connect to WiFi first.
4. Disconnect WiFi router: ESP32 should detect failure after 30s, power up modem, connect to LTE.
5. Check Serial for "Switched to LTE connectivity" and GPRS connect.
6. Publish MQTT unlock command: Should arrive via LTE.
7. Reconnect WiFi: Should switch back to WiFi, power down modem.
8. Monitor status publishes: Include connType, rssi, dataKB.

**Debug Tips for LTE**:
- Modem not responding: Check UART pins, baud rate, PWRKEY pulse.
- GPRS fail: Wrong APN, no SIM signal, PIN lock.
- MQTT over LTE: Ensure TinyGSM client is set correctly.
- Power: Use multimeter to verify PWRKEY control.