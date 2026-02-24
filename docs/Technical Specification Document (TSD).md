- # Technical Specification Document (TSD)

  **Project**: WATERFRONT -> Kayak Rental Booking Application with ESP32-Cashless Controller Integration (incl. BTC/Lightning Payments)

  **Document Title**: Technical Specification Document (TSD)

  **Version**: 1.2 (Updated 23 Feb 2026)

  **Date**: February 23, 2026

  **Author**: Grok AI (xAI) – for BangLee (@BangLee8888), Bremen, DE

  **Changes in 1.2**: Revised for scratch implementation cloning RentalBuddy frontend and functionality; removed bookcars references; emphasized Supabase + Next.js starters for modular build.

  This TSD now fully aligns with **RentalBuddy** (self-service 24/7 unmanned rentals, Stripe integration, digital lock/PIN access, ID checks, website embedding, reminders) and **HEIUKI Share** (heiuki.com / app.heiuki.com – automated SUP/kayak vending machines, book-online → open-compartment → take → paddle → return-to-machine flow, solar-powered lockers, booking-number entry, centralized smartphone management, Progressive Web App).

  ## 1. Overview (Updated)

  ### 1.1 Rental Process – Full Flow (HEIUKI + RentalBuddy inspired)

  1. **Book online** (web/PWA) → Equipment and Calendar availability, time slot, add-ons (life jacket, etc.), optional ID/document upload, Terms&Conditions confirmation.
  2. **Pay** (Stripe fiat AND BTCPay Lightning/Liquid BTC).
  3. **Receive booking confirmation** → Instant payment receipt and booking number/details.
  4. **Receive access** → PIN/QR + booking details 10 minutes before start of rental time.
  5. **At machine** → Open confirmation link / scan QR in smartphone browser → PWA loads access screen → User taps "Unlock Now" button → Backend validates and publishes MQTT unlock command → ESP32 opens compartment/lock. (Optional future enhancement: Enter 4–6 digit PIN directly on physical keypad at machine → local ESP32 validation & unlock)
  6. **Take equipment** → Sensors detect removal → ESP32 publishes “taken” event.
  7. **Use** → Customer paddles (no on-site staff).
  8. **Return** → Put kayak back into bay → Sensors confirm presence → ESP32 auto-locks + publishes “returned”.
  9. **Post-return** → Auto-release deposit (if any), send receipt/reminder.

  **Edge Cases Covered** (from HEIUKI GTC/FAQ + RentalBuddy logic):

  - Late return → Auto-charge extra time (via MQTT command + Stripe/BTCPay).
  - No-show / cancellation → Auto-refund (policy-based) + slot release.
  - Damage / missing equipment → Customer reports via app (photo upload) → Admin alert + deposit hold.
  - Overdue → Lock remains closed until manual admin override or payment.
  - Weather / force-majeure → Admin dashboard bulk-cancel + refund.
  - Low battery / offline → ESP32 queues events + fallback local PIN validation (pre-synced list).
  - ID verification required → Optional during checkout (upload scan → stored securely).

  ## 2. Technology Stack

  | Layer                 | Technology                                                   | Notes / BTC Integration                                      |
  | --------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ |
  | Payments              | Stripe (fiat) + **BTCPay Server** (BTC/Lightning + Liquid)   | Self-hosted, non-custodial, zero platform fees; Lightning invoices + on-chain; Liquid via BTCPay plugins/extensions |
  | Payment Webhooks      | BTCPay Greenfield API + Stripe Webhooks                      | Instant confirmation → MQTT unlock                           |
  | ESP32 Firmware        | Arduino/ESP-IDF + ArduinoJson + PubSubClient                 | + Return sensor logic + deposit-release handling             |
  | WiFi Provisioning     | ESP-IDF WiFi Provisioning Component (BLE or SoftAP) + optional ESPAsyncWebServer for captive portal | Enables runtime SSID/password change without re-flashing. BLE preferred for low power & security. SoftAP fallback for browser-based config. |
  | Cellular Connectivity | LTE modem module (e.g., SIM7600/SIM7500, Quectel EC21, or integrated like Walter board) via UART/SPI | Provides fallback when WiFi is unavailable. Prefer LTE-M / NB-IoT for low power/data usage in vending/IoT. |
  | Cellular Driver       | TinyGSM library (Arduino) or ESP-IDF modem driver            | Handles AT commands, PDP context, MQTT over cellular. Supports SIM7600, SIM800, Quectel, etc. |
  | Connectivity Manager  | Custom state machine (WiFi → LTE failover)                   | Monitors RSSI/link status; switches transparently; reports via MQTT telemetry. |

  **BTCPay Server Setup** (recommended for privacy & DE compliance):

  - Self-host on same VPS as Mosquitto (Docker compose).
  - Generate Lightning invoices via BTCPay API on payment success.
  - Optional Liquid BTC (faster/cheaper sidechain) via BTCPay’s altcoin support or Coinsnap fallback.
  - Webhook → Backend → MQTT /kayak/{machineId}/unlock on confirmed payment.

  ## 3. Updated MQTT Topic Structure

  ### New/Extended Topics (for full HEIUKI-style return flow)

  | Topic                             | Direction       | QoS  | Payload Example (JSON)                                 | Trigger / Action                     |
  | --------------------------------- | --------------- | ---- | ------------------------------------------------------ | ------------------------------------ |
  | /kayak/{machineId}/unlock         | Backend → ESP32 | 1    | {"bookingId":"bk_abc","pin":"7482","durationSec":7200} | Payment confirmed (Stripe or BTCPay) |
  | /kayak/{machineId}/returnConfirm  | Backend → ESP32 | 1    | {"bookingId":"bk_abc","action":"autoLock"}             | Late return or admin command         |
  | /kayak/{machineId}/status         | ESP32 → Backend | 1    | {"state":"returned","kayakPresent":true,"battery":87}  | Sensor detects return                |
  | /kayak/{machineId}/event          | ESP32 → Backend | 1    | {"event":"taken","bookingId":"bk_abc"}                 | Equipment removal                    |
  | /kayak/{machineId}/depositRelease | Backend → ESP32 | 1    | {"bookingId":"bk_abc","release":true}                  | Successful timely return             |

  ## 4. ESP32 Firmware Extensions (nodestark base)

  **4.1 New modules** (add to previous structure):

  - return_sensor.cpp → Ultrasonic / magnetic / weight sensors per bay (detect kayak presence).
  - deposit_handler.cpp → Hold/release logic (LED blink or secondary lock if deposit pending).
  - offline_fallback.cpp → Pre-load recent PIN list via MQTT retained message; validate locally if broker unreachable.
  - pin_validator.cpp (optional – for phase 2 keypad support) → Compare entered PIN against pre-loaded list from retained topic /kayak/{machineId}/pins or local NVS cache. Only compile/activate when keypad hardware is present.
  
  **Note on unlock variants**
  
  - **MVP / primary path**: Remote unlock via PWA button → MQTT command. ESP32 only needs to subscribe to /unlock topic and act on valid payloads. No local PIN input hardware required.
  - **Phase 2 optional path**: Add support for local keypad entry. Requires:
    - Hardware: I²C or matrix keypad + small OLED/e-ink display
    - Software: GPIO input handling, PIN comparison logic, fallback unlock if MQTT unreachable
    - Retained message strategy: Backend publishes current active PINs as retained message on /kayak/{machineId}/pins (array or JSON)
  
  **4.2 Power / Solar Optimisation** (HEIUKI-style autonomous machines):
  
  - Deep sleep 99% of time.
  - Wake on MQTT (external interrupt) or timer (every 15 min for status).
  
  **4.3 WiFi Provisioning & Runtime Configuration**
  
  - Use Espressif's official **wifi_provisioning** component (built into ESP-IDF).
    - Preferred scheme: **BLE transport** (wifi_prov_scheme_ble) – low power, secure, works with smartphone apps (nRF Connect, LightBlue, or custom).
    - Fallback scheme: **SoftAP + HTTP + captive portal** (wifi_prov_scheme_softap) – operator connects to temporary AP ("KayakMachine-Setup"), browser opens config page automatically.
  - Trigger provisioning mode:
    - On first boot if no credentials in NVS (wifi_prov_mgr_is_provisioned() returns false).
    - On operator command (physical button long-press → GPIO interrupt → enter provisioning).
    - Optional: MQTT admin command (/kayak/{machineId}/provision/start) – requires strong auth.
  - Storage: Save credentials to **NVS** namespace "wifi" using nvs_set_str() / nvs_commit().
  - Post-provisioning:
    - Stop provisioning service.
    - Restart WiFi in station mode with new credentials.
    - Reconnect MQTT.
    - Publish status update: {"wifiState":"connected","ssid":"NewNetwork","ip":"192.168.1.123"}
  - Security:
    - BLE: Use secure pairing (just-works or numeric comparison).
    - SoftAP: WPA2-PSK with generated random password shown on device LED/display or in admin log.
    - Rate-limit / timeout provisioning mode to prevent abuse.
  
  **4.4 Cellular Connectivity & Failover**
  
  - **Hardware integration**: LTE module connected via UART2 (TX/RX pins, e.g., GPIO16/17) + PWRKEY pin for power control + optional STATUS pin.
    - Power: Use MOSFET or dedicated enable pin to turn modem off when not needed (critical for solar).
    - SIM: micro/nano SIM slot; support PIN unlock if required.
  - **Software** — Use TinyGSM library (Arduino) or native ESP-IDF PPP/LwIP for cellular IP stack.
    - Initialization: On boot or failover, gsm.begin() → set APN → connect GPRS/LTE.
    - MQTT over cellular: Same PubSubClient instance — TinyGSM provides transparent TCP client.
  - **Failover logic** (simple state machine):
    1. Try WiFi connect (timeout 30 s).
    2. If fail → power up LTE modem → connect cellular → retry MQTT.
    3. If both fail → deep sleep + retry after 15 min.
    4. Prefer WiFi when available (lower power, no data cost).
  - **Telemetry additions**: Publish { "connType": "LTE", "rssi": -78, "apn": "internet", "dataUsedKB": 124 } every 30 min or on change.
  - **Power optimization**:
    - Use LTE-M / NB-IoT modes (lower power than full LTE).
    - Power-down modem (gsm.sleep() or PWRKEY toggle) when connected to WiFi.
    - Only wake modem for critical events (e.g., unlock command timeout).
  
  ## 5. Payment Module – Detailed
  
  ### 5.1 Fiat (Stripe) – RentalBuddy style
  
  - Checkout Session + Webhook on payment_intent.succeeded.
  
  ### 5.2 Bitcoin / Lightning / Liquid (BTCPay)
  
  - On booking confirmation → Create BTCPay Invoice (Lightning preferred for instant confirmation).
  - Webhook InvoicePaid → Backend publishes MQTT unlock.
  - Liquid BTC support: Enable in BTCPay (Liquid sidechain plugin) or fallback to Coinsnap.
  - Settlement options: Keep in BTC/Lightning wallet or auto-convert to EUR via Strike API integration (BTCPay supports it).
  - Zero-fee, non-custodial, on-chain + Lightning + Liquid in one dashboard.

  ### 5.3 Unified Checkout Flow
  
  1. Customer chooses payment method (Card / Klarna / Apple Pay / Lightning BTC / Liquid BTC).
  2. Backend creates invoice (Stripe or BTCPay).
  3. Success → Generate PIN + QR deep-link → Send via email/SMS → Prepare MQTT unlock command (triggered by PWA button press)(Future: Also make PIN available for local keypad entry if hardware is installed)
  4. Deposit handling → Hold via payment metadata; release on sensor-confirmed return.
  
  ## 6. Development Roadmap (with BTC & HEIUKI flow)
  
  1. Supabase project setup + auth + basic DB schema (bookings now include paymentMethod, btcInvoiceId, depositStatus).
  2. Next.js booking UI + calendar + ID upload (optional, HEIUKI/RentalBuddy), cloning RentalBuddy frontend.
  3. Stripe Checkout.
  4. **BTCPay Server Docker** + Lightning invoice creation + webhook.
  5. MQTT broker + publish on any payment success.
  6. ESP32 base (WiFi + MQTT + relay).
  7. **Add LTE cellular fallback**
     - Wire modem (UART + control pins).
     - Integrate TinyGSM library + basic connection test.
     - Implement simple failover state machine (WiFi primary → LTE secondary).
     - Extend status publish with connection type/RSSI.
     - Test: Disable WiFi → verify unlock command still arrives via LTE.
  8. **Add runtime WiFi provisioning** (critical for production usability)
     - Implement BLE provisioning first (copy from ESP-IDF examples: wifi_prov_scheme_ble).
     - Test: Flash with no credentials → use nRF Connect app to send SSID/password → verify connection.
     - Add physical button trigger (debounce + long-press detection).
     - Later add SoftAP fallback if BLE not sufficient.
  9. **Return sensors + confirm logic** (critical for HEIUKI-style auto-locking).
  10. Admin dashboard with real-time telemetry + BTC payment logs.
  11. Edge-case handlers (late return, damage report, no-show refund).
  12. Production deployment + TLS everywhere.
  
  **Sub-Steps for Beginners**: Each step includes AI prompts (e.g., "Code Stripe integration with tests"), unit tests, and debug tips.
  
  ## 7. Important References & Starter Repositories
  
  - nodestark ESP32 cashless (starting point): https://github.com/nodestark/mdb-esp32-cashless
  - MQTT best practices: https://www.hivemq.com/blog/mqtt-security-fundamentals-tls-ssl/
  - Supabase + Next.js starter: https://github.com/supabase/supabase/tree/master/examples/nextjs
  - ArduinoJson (essential for ESP32): https://arduinojson.org/
  - BTCPay: https://docs.btcpayserver.org/

  ## 8. Beginner Implementation Guide

  - **Setup**: Install VS Code, Git, Node.js, Arduino IDE; clone repos.
  - **Gradual Coding**: Break into 1-hour tasks; use AI for "Fix this error: [paste]".
  - **Testing/Debugging**: Use Jest/Mocha for backend; Serial Monitor for ESP32; log everything.
  - **Learning**: 1 resource per step (e.g., YouTube "Node.js for Beginners"); ask AI questions.
  - **Checklist**: Per phase, verify with "Does this match FSD FR-XXX?".
  - **Pitfalls**: Test offline modes early; use Git branches for features.

  **WiFi Provisioning for Beginners**

  - Don't implement everything at once.
  - Step 1: Get basic hardcoded WiFi working (use your home router).
  - Step 2: Add wifi_prov_mgr_is_provisioned() check → if false, start BLE provisioning.
  - Step 3: Test with free BLE app (nRF Connect on Android/iOS): write to characteristics → see ESP32 connect.
  - Common pitfalls: BLE advertising too weak → increase TX power; NVS not initialized → call nvs_flash_init().
  - Prompt example for AI: "Integrate ESP-IDF wifi_prov_scheme_ble into existing MQTT project. Include provisioning start on first boot or GPIO button press. Save credentials to NVS. Restart WiFi station mode after success. Provide full code and explanation."
  
  **Adding LTE Cellular (for Beginners)**
  
  - Choose a modem: SIM7600G (global LTE) or SIM7000 (cheaper, LTE-M/NB-IoT) — both widely supported by TinyGSM.
  - Start simple: Test modem alone (AT commands via Serial) → "AT+CSQ" for signal, "AT+CGATT?" for attach.
  - Use TinyGSM examples: GSM_MQTT.ino or MQTT.ino → adapt to your existing MQTT code.
  - Common issues: Wrong APN (e.g., "internet.t-mobile.de" in DE), SIM PIN lock, antenna not connected → use Serial debug.
  - AI prompt example: "Add TinyGSM + SIM7600 LTE failover to existing ESP32 MQTT project. Prefer WiFi, switch to LTE if WiFi fails after 30s. Power control modem via GPIO. Publish connection type in status."