# Waterfront ESP32 MQTT Code Development

### Project Plan for Adapting of Nodestark/mdb-esp32-cashless 

This project plan focuses on the beginner roadmap tie-in for the Waterfront Kayak Rental Application and Control System, as outlined in the TSD (Technical Specification Document) sub-steps. It adapts the Nodestark repo (https://github.com/nodestark/mdb-esp32-cashless) for ESP32-based lock control without local payments or MDB (Multi-Drop Bus) hardware. Since bookings and payments occur via the PWA (Progressive Web App) using Stripe/BTCPay, we'll strip MDB-specific components, add relay and sensor logic for kayak bays, handle JSON payloads over MQTT (e.g., for unlock commands and status events), and integrate with Supabase for the full flow: booking → pay → MQTT unlock → sensor confirm.

The plan is broken into 1-2 hour tasks/goals, assuming beginner-level skills (e.g., basic Git, Arduino IDE/ESP-IDF, and AI tools like Grok or Claude for code generation). Each task includes:
- **Goal**: What to achieve.
- **Estimated Time**: 1-2 hours.
- **Steps**: Actionable sub-steps.
- **AI Prompt Example**: For code assistance.
- **Testing/Tips**: Verification and pitfalls.

Total estimated time: 10-15 hours, spread over days. Prerequisites: Install VS Code, Git, Arduino IDE (or PlatformIO for ESP-IDF), and set up a local MQTT broker (e.g., Mosquitto via Docker). Use Serial Monitor for ESP32 debugging. Align with TSD Section 6 (Development Roadmap) and FSD FR-ESP-01 to FR-ESP-04.

#### General Assignment

The **WATERFRONT** kayak rental ESP32 firmware will be built by heavily adapting the **nodestark/mdb-esp32-cashless** repository. The original project implements an MDB (Multi-Drop Bus) cashless vending device on ESP32(-S3), with strong MQTT integration for payments/telemetry, BLE for local interaction, and modular master/slave architecture.

For **WATERFRONT**, we completely remove MDB vending logic (no local coin/bill/credit card handling), keep and extend the MQTT + JSON handling parts, and add new modules for:

- Relay/solenoid lock control (compartment open/close)
- Sensor-based kayak presence detection (taken/returned events)
- Runtime WiFi provisioning (BLE preferred + SoftAP fallback)
- LTE cellular failover (TinyGSM)
- Offline fallback (pre-synced PIN list in NVS)
- Power optimization (deep sleep, modem power gating)

This keeps proven networking code while aligning exactly with TSD §3 (MQTT topics), §4 (firmware extensions), and §6 (roadmap priorities: provisioning, LTE, sensors, return logic).

#### Proposed Firmware Structure (after adaptation)

waterfront-kayak-esp32/                  # New root project name (fork or new from nodestark base)
├── CMakeLists.txt                       # Root project CMake (idf_component_register style)
├── sdkconfig                            # Project-wide config (via menuconfig)
├── sdkconfig.defaults                   # Default settings (WiFi, MQTT broker, machineId, etc.)
├── partitions.csv                       # Custom partition table if needed (e.g. larger NVS)
├── main/                                # Main application source
│   ├── CMakeLists.txt
│   ├── main.cpp                         # Entry point: setup(), loop(), task creation
│   ├── config.h                         # Global constants (pins, topics, timeouts)
│   ├── wifi_manager.cpp / .h            # WiFi station + provisioning (from nodestark BLE/SoftAP)
│   ├── mqtt_handler.cpp / .h            # MQTT client (PubSubClient), subscribe callback, JSON parse
│   ├── relay_handler.cpp / .h           # Lock control (pulse open/close on GPIO)
│   ├── return_sensor.cpp / .h           # Kayak presence (ultrasonic/magnetic/weight polling)
│   ├── deposit_logic.cpp / .h           # Timer for rental duration, auto-lock, deposit release flag
│   ├── offline_fallback.cpp / .h        # NVS-stored PIN list, local validation if broker down
│   ├── telemetry.cpp / .h               # Periodic status publish (battery, conn type, RSSI)
│   └── power_manager.cpp / .h           # Deep sleep, modem power control, wake sources
├── components/                          # Reusable IDF components (add as needed)
│   ├── tinygsm/                         # TinyGSM for LTE modem (external component or local)
│   └── arduinojson/                     # If not using managed component
├── managed_components/                  # IDF component manager deps (if using)
│   └── espressif__...                   # e.g. arduino-esp32 if hybrid, mqtt, etc.
└── docs/                                # Optional: pinout diagram, wiring notes, flowcharts



**Key architectural decisions**

- **Framework** — Full ESP-IDF (v5.5.3) preferred over Arduino for better power control, NVS, provisioning components, and TinyGSM integration. If nodestark base is Arduino-style, gradually migrate main logic to IDF tasks.

- **Execution model** — FreeRTOS tasks:

  - Main loop → minimal (watchdog feeding)
  - Separate tasks: mqtt_task, sensor_poll_task, power_task, provisioning_task

- **Configuration** — Use menuconfig for compile-time choices (broker URL, machineId, pin numbers, LTE APN, etc.)

- **JSON handling** — ArduinoJson 6/7 exclusively for parsing unlock payloads and building status/event messages

- **Connectivity** — State machine: WiFi → LTE failover → offline mode

- **Security** — MQTT over TLS (later), NVS encrypted if possible, BLE secure pairing

  

#### Task 1: Clone Repo and Setup Development Environment

- **Goal**: Get the Nodestark base code locally and prepare for ESP32 development without MDB dependencies.
- **Estimated Time**: 1 hour.
- **Steps**:
  1. Clone the repo: `git clone https://github.com/nodestark/mdb-esp32-cashless.git` and create a new branch: `git checkout -b waterfront-adaptation`.
  2. Install dependencies: Open in VS Code/Arduino IDE; add libraries like PubSubClient (for MQTT), ArduinoJson (for payloads), and ESP-IDF components if switching from Arduino.
  3. Flash a basic ESP32 test sketch to verify hardware (e.g., blink LED on GPIO).
- **AI Prompt Example**: "Provide a beginner setup guide for cloning and branching the Nodestark/mdb-esp32-cashless repo in VS Code for ESP32 development."
- **Testing/Tips**: Build and upload a simple "Hello World" sketch via Serial Monitor. Tip: If using ESP-IDF, follow Espressif docs for environment setup; avoid MDB folders initially.

#### Task 2: Remove MDB-Specific Folders and Code
- **Goal**: Strip out vending/MDB-related components to simplify for kayak lock control (focus on MQTT and GPIO).
- **Estimated Time**: 1-2 hours.
- **Steps**:
  1. Delete MDB folders: Remove `mdb-master-esp32s3/` and `mdb-slave-esp32s3/` (core MDB protocol logic, irrelevant for PWA-only payments).
  2. Clean up references: In README.md and any shared files, remove mentions of MDB/EVA-DTS/DEX (e.g., telemetry parsing tied to vending).
  3. Retain useful parts: Keep MQTT client code, BLE stack (for potential provisioning), and general ESP32 setup (e.g., WiFi manager).
  4. Commit changes: `git commit -m "Removed MDB-specific folders for Waterfront adaptation"`.
- **AI Prompt Example**: "Help remove MDB-related code from Nodestark/mdb-esp32-cashless repo: Identify and delete vending protocol files while keeping MQTT and BLE for a non-payment ESP32 lock controller."
- **Testing/Tips**: Build the remaining code; check for errors in Serial Monitor (e.g., unresolved MDB includes). Tip: Backup the original repo; search for "MDB" keywords in code to ensure full removal.

#### Task 2.5: Define & Implement New Firmware Structure

- **Goal**: Restructure the cleaned repo into a clean, modular ESP-IDF project layout matching WATERFRONT requirements (MQTT + relay + sensors + provisioning + LTE).
- **Estimated Time**: 1.5–2.5 hours.
- **Steps**:
  1. Create new project root: Rename folder to waterfront-kayak-esp32 or create fresh via idf.py create-project waterfront-kayak-esp32.
  2. Copy useful parts from cleaned nodestark base:
     - WiFi + MQTT client code
     - BLE provisioning (if present)
     - JSON handling helpers
  3. Create folder structure as shown above (main/, components/).
  4. Move/rewrite entry point to main/main.cpp: init NVS → WiFi → MQTT connect → task creation.
  5. Add empty skeleton files for new modules (relay_handler, return_sensor, etc.) with basic headers and placeholders.
  6. Update root CMakeLists.txt to include all source files and components.
  7. Run first build: idf.py build → fix includes/links.
  8. Commit: git commit -m "Introduced WATERFRONT firmware structure with modular MQTT/relay/sensor layout"
- **AI Prompt Example**: "Generate a complete ESP-IDF project structure for a kayak rental lock controller adapted from nodestark/mdb-esp32-cashless (MQTT-based, no MDB). Include folder layout, root CMakeLists.txt snippet, main.cpp skeleton with FreeRTOS tasks for MQTT, sensor polling, relay control, and placeholders for WiFi provisioning, TinyGSM LTE failover, NVS offline PINs."
- **Testing/Tips**: Build must succeed without errors. Use idf.py size to check binary fits. Tip: Start with esp32 target (idf.py set-target esp32); switch to esp32s3 later if more RAM/PSRAM needed for JSON buffers.

#### Task 3: Add Basic Relay Code for Lock Control

- **Goal**: Implement relay control for 12V solenoid locks (e.g., pulse open/close on GPIO), aligning with TSD Section 4 (ESP32 Firmware Extensions).
- **Estimated Time**: 1-2 hours.
- **Steps**:
  1. Choose GPIO: Assign a pin (e.g., GPIO 23) for relay in code (use Arduino/ESP-IDF digitalWrite or gpio_set_level).
  2. Add relay functions: Create a module like `relay_handler.cpp` with unlock (high for 1s) and lock (low) logic.
  3. Integrate with base: Add to the retained ESP32 setup loop; test manual trigger via button or Serial command.
- **AI Prompt Example**: "Add relay code to Nodestark ESP32 base (post-MDB removal) using Arduino/ESP-IDF: Control a 12V solenoid lock on GPIO 23 with pulse functions for unlock/lock in a kayak rental system."
- **Testing/Tips**: Wire a relay/LED to ESP32; use Serial Monitor to send test commands (e.g., "unlock"). Tip: Ensure power safety (use MOSFET for 12V); debug with multimeter.

#### Task 4: Modify Code to Handle JSON Unlock Payloads via MQTT
- **Goal**: Adapt MQTT subscriber to process JSON from backend (e.g., {"bookingId":"bk_abc","pin":"7482","durationSec":7200}) and trigger relay.
- **Estimated Time**: 1-2 hours.
- **Steps**:
  1. Use ArduinoJson: Parse incoming MQTT messages in callback function.
  2. Update topics: Subscribe to `/kayak/{machineId}/unlock` (per TSD Section 3).
  3. Add logic: On valid JSON, validate PIN (simple check), trigger relay unlock, and set timer for duration.
  4. Publish response: Send status back to `/kayak/{machineId}/status` (e.g., {"state":"unlocked"}).
- **AI Prompt Example**: "Modify Nodestark ESP32 code to handle JSON unlock payloads and sensor events without MDB: Use ArduinoJson to parse MQTT messages like {'bookingId':'bk_abc','pin':'7482'} and trigger a relay unlock."
- **Testing/Tips**: Use a local MQTT broker; publish test JSON via mosquitto_pub. Monitor Serial for parsing logs. Tip: Handle errors (e.g., invalid JSON) with retries.

#### Task 5: Add Sensor Events for Kayak Presence Detection
- **Goal**: Integrate sensors (e.g., ultrasonic HC-SR04) for "taken" and "returned" events, enabling auto-lock and deposit release.
- **Estimated Time**: 1-2 hours.
- **Steps**:
  1. Wire sensor: Assign GPIOs (e.g., trigger 5, echo 18).
  2. Add module: Create `return_sensor.cpp` to detect presence (distance threshold for kayak in bay).
  3. Event handling: In loop, check sensor; publish MQTT events like `/kayak/{machineId}/event` ({"event":"taken"}) or `/kayak/{machineId}/status` ({"kayakPresent":true}).
  4. Link to relay: Auto-lock on return detection.
- **AI Prompt Example**: "Extend Nodestark ESP32 MQTT code with ultrasonic sensor (HC-SR04) for kayak presence: Publish JSON events on detection and auto-lock relay for Waterfront rental flow."
- **Testing/Tips**: Simulate with objects; view Serial output for distance readings. Tip: Debounce readings to avoid false triggers; calibrate threshold for kayak size.

#### Task 6: Test MQTT Unlocks Locally with Serial Monitor
- **Goal**: Verify end-to-end ESP32 logic (MQTT receive → JSON parse → relay/sensor action) without backend.
- **Estimated Time**: 1 hour.
- **Steps**:
  1. Flash updated firmware to ESP32.
  2. Connect to local MQTT broker; subscribe/publish test messages.
  3. Simulate unlock: Send JSON payload; observe relay pulse and sensor event publishes.
  4. Edge cases: Test invalid PIN or offline (queue messages).
- **AI Prompt Example**: "Debug script for testing Nodestark-adapted ESP32 MQTT unlocks: Provide mosquitto commands and Serial Monitor checks for JSON handling and relay/sensor."
- **Testing/Tips**: Use Serial Monitor for logs (e.g., "Received unlock for booking bk_abc"). Tip: Monitor MQTT traffic with tools like MQTT Explorer.

#### Task 7: Integrate with Supabase for Booking and Payment Flow
- **Goal**: Connect ESP32 to Supabase backend for real bookings (PWA → pay → webhook → MQTT unlock).
- **Estimated Time**: 2 hours.
- **Steps**:
  1. Setup Supabase: Create tables for bookings (per TSD Section 6: DB schema with paymentMethod, btcInvoiceId).
  2. Add webhooks: Configure Stripe/BTCPay to hit Supabase functions that publish MQTT on success.
  3. Test partial flow: Book via local PWA → simulate pay → MQTT unlock → ESP32 relay/sensor confirm → update DB status.
- **AI Prompt Example**: "Integrate Nodestark ESP32 MQTT with Supabase for Waterfront flow: Code webhook to publish JSON unlock on payment, and handle sensor confirm events."
- **Testing/Tips**: Use Supabase local CLI for dev; log webhook hits. Tip: Start with fiat (Stripe) before BTC; ensure TLS for MQTT.

#### Task 8: Full End-to-End Flow Testing and Refinements
- **Goal**: Validate complete cycle (booking → pay → MQTT unlock → sensor confirm) with edge cases.
- **Estimated Time**: 1-2 hours.
- **Steps**:
  1. Run PWA locally (Next.js setup from TSD).
  2. Test flow: Book slot → pay (test mode) → unlock → simulate take/return → confirm deposit release via MQTT.
  3. Add refinements: Offline fallback (pre-sync PINs), power optimization (deep sleep).
  4. Commit and deploy: Push to Git; flash production ESP32.
- **AI Prompt Example**: "Test plan for full Waterfront flow using adapted Nodestark ESP32: Debug booking → pay → MQTT unlock → sensor confirm with Supabase integration."
- **Testing/Tips**: Use Serial Monitor and Supabase console for end-to-end logs. Tip: Handle Bremen-specific edge cases (e.g., weather cancels via admin MQTT); iterate with AI for fixes.

This plan aligns with HEIUKI/RentalBuddy flows (TSD Section 1.1) and emphasizes modular ESP32 MQTT development. Track progress in Git; if stuck, use AI for error fixes (e.g., "Fix this ESP32 compile error: [paste]"). Next steps: Extend to WiFi provisioning and LTE (TSD Section 4.3-4.4).