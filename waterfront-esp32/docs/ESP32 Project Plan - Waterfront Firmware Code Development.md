# ESP32 Project Plan - Waterfront Firmware Code Development

**Project**: WATERFRONT – Kayak / SUP Rental Vending Machine Controller  
**Firmware Target**: ESP32 (Arduino framework via PlatformIO)  
**Current Status**: PlatformIO baseline restored (March 2026) – MQTT connect + status publish working, topics updated to `/station/`  
**Goal**: Self-hosted, solar-powered, unmanned lock control with sensor confirmation, WiFi + LTE failover, runtime provisioning  
**Base**: Adapted from nodestark/mdb-esp32-cashless + custom additions for HEIUKI/RentalBuddy flow

## 1. Technology & Build Stack (Updated 2026)

- **Framework**: Arduino core for ESP32 (not pure ESP-IDF)
- **Build System**: PlatformIO (VS Code extension) – preferred for beginners (auto toolchain, lib management, IntelliSense)
- **Board**: ESP32 Dev Module (esp32dev)
- **Core Libraries**:
  - PubSubClient (MQTT)
  - ArduinoJson (payloads)
  - TinyGSM (LTE failover – SIM7600 / SIM7000 / Quectel)
  - NimBLE-Arduino (planned for BLE WiFi provisioning)
- **platformio.ini** key settings:
  - platform = platformio/espressif32
  - lib_deps: PubSubClient, ArduinoJson, TinyGSM (NimBLE later)
  - build_flags: verbose debug, SPI auto-init, JSON std::string support

## 2. Current Working Features (March 2026 baseline)

- WiFi station mode with fallback credentials (NVS later)
- MQTT connect to broker (test.mosquitto.org or self-hosted Mosquitto)
- Subscribe to:
  - `/station/{machineId}/unlock`
  - `/station/{machineId}/returnConfirm`
  - `/station/{machineId}/depositRelease`
- Publish to:
  - `/station/{machineId}/status` (periodic + retained online announce)
  - `/station/{machineId}/event` (future taken/returned)
- Basic relay pulse on unlock command (LED + lock GPIO)
- Serial debug output

## 3. MQTT Topic Structure (Final – aligned with backend)

Prefix changed to `/station/` for generality (not product-specific like `/kayak/`).

| Topic                               | Direction     | QoS  | Payload Example (JSON)                                | Purpose                           |
| ----------------------------------- | ------------- | ---- | ----------------------------------------------------- | --------------------------------- |
| /station/{machineId}/unlock         | Backend → ESP | 1    | {"bookingId":"bk123","pin":"7482","durationSec":7200} | Payment confirmed → open lock     |
| /station/{machineId}/returnConfirm  | Backend → ESP | 1    | {"bookingId":"bk123","action":"autoLock"}             | Force lock or late-return command |
| /station/{machineId}/depositRelease | Backend → ESP | 1    | {"bookingId":"bk123","release":true}                  | Timely return → release deposit   |
| /station/{machineId}/status         | ESP → Backend | 1    | {"state":"connected","uptimeMin":45,"rssi":-68}       | Telemetry (periodic + changes)    |
| /station/{machineId}/event          | ESP → Backend | 1    | {"event":"taken","bookingId":"bk123"}                 | Kayak removed / returned          |

Retained messages used for status/online state.

## 4. Development Roadmap & Priorities

1. **Done – PlatformIO Baseline** (March 2026)
   - Clean build, serial output, WiFi connect
   - MQTT reconnect + subscribe/publish
   - Topic prefix `/station/`

2. **Next – Runtime WiFi Provisioning (BLE preferred)**
   - Use NimBLE-Arduino + ESP32 WiFi provisioning service
   - Trigger: first boot (no NVS creds) or long-press GPIO button
   - Schemes: BLE (primary), SoftAP + captive portal (fallback)
   - Store SSID/password in NVS
   - Publish success status via MQTT

3. **LTE Cellular Failover**
   - Integrate TinyGSM (UART2 + PWRKEY control)
   - State machine: WiFi → timeout → power-up modem → LTE → MQTT
   - Telemetry: connType ("WiFi"/"LTE"), rssi, data usage estimate
   - Power optimization: modem sleep/off when WiFi ok

4. **Return Sensor & Confirmation Logic**
   - Ultrasonic (HC-SR04) or magnetic reed + weight/pressure per bay
   - Detect "taken" → publish /event {"event":"taken"}
   - Detect "returned" → publish /event {"event":"returned"} + auto-lock
   - Timeout handling (no return → late fee via backend)

5. **Deposit & Overdue Handling**
   - Hold deposit flag (MQTT command)
   - LED blink / secondary lock if overdue
   - Release on confirmed return + timely

6. **Offline Fallback**
   - Pre-sync recent PIN list via retained MQTT message
   - Local PIN validation if broker unreachable

7. **Power & Solar Optimizations**
   - Deep sleep 99% (wake on timer / external interrupt / MQTT)
   - Battery voltage monitoring (ADC)
   - Status publish includes battery %

8. **Testing & Diagnostics**
   - Unit tests (Catch2 via PlatformIO test)
   - Serial + MQTT logging levels
   - Over-the-air (OTA) update support (ArduinoOTA lib)

## 5. Hardware Pin Mapping (Tentative)

- Relay / Solenoid lock: GPIO 5
- Status LED: GPIO 2
- Provisioning button (long press): GPIO 0
- Ultrasonic Trigger / Echo: GPIO 18 / 19 (example)
- LTE modem UART: GPIO 16 (TX), 17 (RX) + PWRKEY GPIO 4
- Battery voltage divider: GPIO 34 (ADC)

## 6. Tips for Beginners

- Always open project via PlatformIO Home → Open → select waterfront-esp32 folder
- After changes to platformio.ini: PlatformIO → Rebuild IntelliSense Index
- Build errors? Check TERMINAL tab, not just PROBLEMS panel
- Test MQTT early: Use mqtt-explorer desktop app to publish unlock commands
- Commit often on feature branches (e.g. feat/ble-provisioning)
- Ask for incremental code: "Add BLE provisioning module" / "Implement ultrasonic return sensor"

## 7. References

- PlatformIO docs: https://docs.platformio.org
- PubSubClient: https://github.com/knolleary/pubsubclient
- TinyGSM examples: https://github.com/vshymanskyy/TinyGSM
- NimBLE provisioning: ESP-IDF WiFiProv examples adapted to Arduino
- TSD.md / FSD.md (root docs)

Last updated: March 17, 2026 – PlatformIO revival complete



## 9. OTA (Over-The-Air) Updates

**Goal**: Enable remote firmware updates for deployed ESP32 controllers without physical access (critical for off-grid/solar machines at waterfront locations).  
**Priority**: Medium – implement after basic MQTT + sensors are stable (to avoid bricking during testing).  
**Library**: ArduinoOTA (built-in to espressif32 Arduino framework – no extra lib_deps needed)

### 9.1 Configuration in platformio.ini

Add these lines to your `[env:esp32dev]` section (or create a separate env like `[env:ota]` for testing):

```ini
; OTA support
upload_protocol = espota
; upload_port = mywaterfront.local     ; or specific IP e.g. 192.168.5.123 – set per device or use mDNS
upload_port = 192.168.0.255               ; broadcast for discovery (works if mDNS not set)
upload_flags =
    --port=3232                           ; default OTA port
    --auth=your_secure_password_2026      ; REQUIRED for production – set a strong password
```

- For mDNS discovery (recommended): Use ArduinoOTA.setHostname("waterfront-" MACHINE_ID) in code → upload with pio run -t upload --upload-port waterfront-kayak-bremen-01.local
- Test upload: After flashing initial firmware via USB, use pio run -t upload with --upload-port set.

### 9.2 Code Integration (add to src/main.cpp)

Include and initialize ArduinoOTA in setup() and loop().

C++

```
// Add near top (after other includes)
#include <ArduinoOTA.h>

// In setup() – after WiFi is connected
void setup() {
  // ... existing WiFi connect code ...

  if (WiFi.status() == WL_CONNECTED) {
    // OTA Configuration
    ArduinoOTA.setHostname(("waterfront-" MACHINE_ID).c_str());  // e.g. waterfront-kayak-bremen-01.local
    ArduinoOTA.setPassword("your_secure_password_2026");         // match upload_flags --auth

    // Optional: customize port if not 3232
    // ArduinoOTA.setPort(3232);

    // Callbacks for progress/feedback (optional but useful)
    ArduinoOTA.onStart([]() {
      Serial.println("OTA Start");
      // Optional: stop sensors/MQTT temporarily if needed
    });
    ArduinoOTA.onEnd([]() {
      Serial.println("\nOTA End");
      ESP.restart();
    });
    ArduinoOTA.onProgress(<a href="unsigned int progress, unsigned int total" target="_blank" rel="noopener noreferrer nofollow"></a> {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError(<a href="ota_error_t error" target="_blank" rel="noopener noreferrer nofollow"></a> {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

    ArduinoOTA.begin();
    Serial.println("OTA Ready – hostname: " + String(ArduinoOTA.getHostname()));
  }
}

// In loop() – add this call (non-blocking)
void loop() {
  // ... existing MQTT loop, status publish etc. ...

  ArduinoOTA.handle();  // MUST be called regularly – processes OTA requests
}
```

### 9.3 Security & Production Notes

- **Always use password**: Set --auth in platformio.ini + matching setPassword() in code.
- **mDNS**: Enables easy discovery (hostname.local) – works on most home/office networks.
- **Disable in deep sleep**: ArduinoOTA.handle() is lightweight, but if power is ultra-critical, call it only when connected and not sleeping.
- **Risk mitigation**:
  - OTA only active after WiFi connect
  - Use strong, per-device passwords (or derive from MACHINE_ID)
  - Consider VPN or firewall for production broker network
- **Alternative libraries** (for later):
  - **ElegantOTA** → Modern web UI (/update page) – add via lib_deps if you want browser-based updates.
  - **HTTPUpdate** → Pull .bin from HTTPS server on MQTT command (e.g., admin sends "update" → ESP downloads from your VPS).

### 9.4 Testing OTA

1. Flash initial firmware via USB.
2. Check serial: "OTA Ready – hostname: waterfront-..."
3. From terminal: pio run -t upload --upload-port waterfront-kayak-bremen-01.local (or IP)
4. Make a small code change (e.g. add Serial.println("OTA test v2")) → upload OTA → verify new message in serial.

### 9.5 Integration with Waterfront Flow

- Trigger OTA via admin dashboard: Publish MQTT command to new topic /station/{machineId}/ota/update → ESP subscribes → calls OTA logic or restarts into update mode.
- Future: Version check via MQTT status publish (include "fwVersion": "1.2.3").

Implement after MQTT stability is confirmed – OTA is powerful but can brick if interrupted during update (rare with good WiFi).

### Quick Summary of Changes
- Uses **built-in ArduinoOTA** (zero extra deps)
- PlatformIO OTA upload via `espota` protocol
- mDNS hostname + password for discovery & security
- Callbacks for progress/error logging
- Ready for MQTT-triggered extension later



Add this section to your document, commit it, then test the code snippet in your `main.cpp`. Once OTA works (first successful wireless upload), reply "OTA tested OK" – we can then add BLE provisioning or LTE failover next.

Let me know if you want a variant (e.g. ElegantOTA web UI instead) or immediate code diff for your current main.cpp!