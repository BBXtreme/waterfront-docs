- # WATERFRONT ESP32 Deployment Guide (Pure ESP-IDF)

  ## What is Deployment?
  Deployment means installing and configuring the WATERFRONT ESP32 Kayak Rental Controller at your real kayak rental location.  
  This guide is written for beginners and assumes you are using **pure ESP-IDF** (Espressif's official development framework) — no PlatformIO, no Arduino core.

  ## Before You Start (Prerequisites)

  1. **Hardware**
     - ESP32 development board (e.g. ESP32-DevKitC, NodeMCU-32S, or any ESP32-WROOM-32 module)
     - Sensors: ultrasonic (HC-SR04), limit switches, current/voltage sensors for solar/battery
     - Actuators: relays or solenoids for locks, servo for gate (if used)
     - Power: solar panel + charge controller + LiFePO4/Li-ion battery
     - USB cable (for flashing & serial monitor)

  2. **Computer Setup**
     - macOS / Windows / Linux (macOS used in examples)
     - ESP-IDF v5.5.3 installed (recommended: git clone or official installer)
       → Your path: `/Users/marco/.espressif/v5.5.3/esp-idf` or `/Users/marco/esp/esp-idf`
     - Python 3.9–3.11 (included in ESP-IDF installer)
     - Git (for updating IDF if needed)

  3. **MQTT Broker**
     - Public test broker (for development): `broker.emqx.io` port 1883 (no auth)
     - Production: your own Mosquitto / EMQX / HiveMQ server (TLS + username/password recommended)

  4. **Certificates** (for secure MQTT / HTTPS OTA)
     - Root CA certificate (e.g. Let's Encrypt, DigiCert) → save as `.pem` file
     - Optional: client certificate + key if your broker requires mutual TLS

  5. **Tools**
     - Terminal (Terminal.app on macOS, PowerShell / cmd on Windows)
     - Text editor: VS Code + Espressif IDF extension, or nano/vim/Zed/BBEdit
     - Serial monitor: `idf.py monitor`, CoolTerm, PuTTY, or screen/minicom

  ## Step-by-Step Deployment

  ### Step 1 – Prepare ESP-IDF Environment

  1. Open Terminal  
  2. Activate ESP-IDF (run this in every new terminal):

     ```bash
     . /Users/marco/.espressif/tools/activate_idf_v5.5.3.sh
     # or if you cloned manually:
     # . /Users/marco/esp/esp-idf/export.sh

  3. Verify installation:

​	Bash

```
idf.py --version
```

​	→ Should show ESP-IDF v5.5.3

### Step 2 – Clone / Update Project

Bash

```
cd ~/Documents/GitHub
git clone https://github.com/yourusername/Waterfront.git   # if not already cloned
cd Waterfront/waterfront-esp32
git pull origin main   # get latest code
```

### Step 3 – Configure Project Settings

1. Open configuration menu:

   Bash

   ```
   idf.py menuconfig
   ```

2. Important settings to enable / adjust:

   - **Component config → ESP HTTPS OTA → Enable**
   - **Component config → SPIFFS → Enable** (for config.json, logs)
   - **Component config → NVS → Enable** (for persistent keys)
   - **Component config → Wi-Fi → Wi-Fi Provisioning → Enable BLE + SoftAP**
   - **Component config → MQTT → Enable**
   - **Serial flasher config → Default serial port** → set to /dev/cu.usbserial-XXXX (check with ls /dev/cu.*)
   - **Partition Table → Custom partition table CSV** (if you need custom sizes)

3. Save (S) → Exit (Q) → confirm

### Step 4 – Build the Firmware

Bash

```
idf.py fullclean    # optional – only if previous build was broken
idf.py build
```

→ Look for [100%] Built target waterfront-esp32 If errors → read the message carefully (most common: missing header → add to main/CMakeLists.txt REQUIRES)

### Step 5 – Flash to ESP32

1. Connect ESP32 via USB

2. Find serial port:

   Bash

   ```
   ls /dev/cu.*
   ```

   Usually /dev/cu.usbserial-XXXX or /dev/cu.SLAB_USBtoUART

3. Flash & open monitor:

   Bash

   ```
   idf.py -p /dev/cu.usbserial-XXXX flash monitor
   ```

   → Press reset button on ESP32 if it hangs

### Step 6 – First Boot & Provisioning

1. Watch serial output (idf.py monitor):
   - Boot log, NVS init, WiFi provisioning start
   - If BLE provisioning enabled → use nRF Connect app (iOS/Android) to connect to “WATERFRONT-PROV”
   - Enter your WiFi SSID/password → device connects to internet
2. Check MQTT connection:
   - Use MQTT Explorer → connect to broker → subscribe to waterfront/#
   - You should see waterfront/status messages

### Step 7 – OTA & Production Setup

1. Upload firmware.bin to your server (HTTPS)

2. Update OTA URL in config.json or via MQTT command

3. Trigger OTA:

   Bash

   ```
   # or send MQTT message to trigger ota_perform_update()
   ```

4. Device downloads, verifies, reboots into new version

### Troubleshooting

- **ESP32 not recognized** → different USB cable/port, hold BOOT button while connecting
- **Build fails on missing header** → add component to REQUIRES in main/CMakeLists.txt (e.g. mqtt, app_update, esp_https_ota)
- **WiFi fails** → check SSID/password, signal strength, idf.py menuconfig → WiFi → max TX power
- **MQTT fails** → verify broker IP/port/username/password, firewall, TLS settings
- **OTA fails** → check HTTPS URL, cert pinning, firewall, enough heap
- **Low power / deep sleep issues** → measure current with multimeter, check esp_sleep_enable_timer_wakeup()

### Tips for Success

- Start small: test one compartment lock/unlock first
- Label all wires and pins (take photos!)
- Keep backups of sdkconfig and config.json
- Use idf.py size to check flash/RAM usage
- Monitor with idf.py monitor or screen /dev/cu.usbserial-XXXX 115200
- Use QEMU for quick testing: idf.py qemu
- Document MQTT topics & payloads early

Good luck deploying WATERFRONT at your location! If stuck → share the exact error from idf.py build or monitor output.

Last updated: March 2026
