# Technical Specification Document (TSD)

**Project**: WATERFRONT -> Kayak Rental Booking Application with ESP32-Cashless Controller Integration (incl. BTC/Lightning Payments)

**Document Title**: Technical Specification Document (TSD)

**Version**: 1.3 (Updated 28 Feb 2026)

**Date**: February 28, 2026

**Author**: Grok AI (xAI) – for BangLee (@BangLee8888), Bremen, DE

**Changes in 1.3**: Marked all ESP32 firmware tasks as completed; added CI/CD with GitHub Actions; full centralized config achieved; overdue timer + watchdog implemented.

This TSD now fully aligns with **RentalBuddy** (self-service 24/7 unmanned rentals, Stripe integration, digital lock/PIN access, ID checks, website embedding, reminders) and **HEIUKI Share** (heiuki.com / app.heiuki.com – automated Equipment (SUP/Kayak) vending machines, book-online → open-compartment → take → paddle → return-to-machine flow, solar-powered lockers, booking-number entry, centralized smartphone management, Progressive Web App).

Type Safety → Zod schemas are the single source of truth. No raw interfaces allowed.

## 1. Overview (Updated)

### 1.1 Rental Process – Full Flow (HEIUKI + RentalBuddy inspired)

1. **Book online** (web/PWA) → Equipment and Calendar availability, time slot, add-ons (life jacket, etc.), optional ID/document upload, Terms&Conditions confirmation.
2. **Pay** (Stripe fiat AND BTCPay Lightning/Liquid BTC).
3. **Receive booking confirmation** → Instant payment receipt and booking number/details.
4. **Receive access** → PIN/QR + booking details 10 minutes before start of rental time.
5. **At location** → Open confirmation link / scan QR in smartphone browser → PWA loads access screen → User taps "Unlock Now" button → Backend validates and publishes MQTT unlock command → ESP32 opens compartment/lock. (Optional future enhancement: Enter 4–6 digit PIN directly on physical keypad at location → local ESP32 validation & unlock)
6. **Take equipment** → Sensors detect removal → ESP32 publishes “taken” event.
7. **Use** → Customer paddles (no on-site staff).
8. **Return** → Put Equipment back into bay → Sensors confirm presence → ESP32 auto-locks + publishes “returned”.
9. **Post-return** → Auto-release deposit (if any), send receipt/reminder.

**Edge Cases Covered** (from HEIUKI GTC/FAQ + RentalBuddy logic):

- Late return → Auto-charge extra time (via MQTT command + Stripe/BTCPay).
- No-show / cancellation → Auto-refund (policy-based) + compartment release.
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
| WiFi Provisioning     | ESP-IDF WiFi Provisioning Component (BLE or SoftAP) + optional ESPAsyncWebServer for captive portal | Enables runtime WiFi SSID/password change without re-flashing. BLE preferred for low power & security. |
| Cellular Connectivity | LTE modem module (e.g., SIM7600G-H / SIM7500G, Quectel EC21, or integrated like Walter board) via UART/SPI | Provides fallback when WiFi is unavailable. Prefer LTE-M / NB-IoT for low power/data usage in vending/IoT. |
| Cellular Driver       | TinyGSM library (Arduino) or ESP-IDF modem driver            | Handles AT commands, PDP context, MQTT over cellular. Supports SIM7600, SIM800, etc. |
| Connectivity Manager  | Custom state machine (WiFi → LTE failover)                   | Monitors RSSI/link status; switches transparently; reports via MQTT telemetry. |
| MQTT Security         | Username/password authentication and TLS encryption (MQTTS) | Secure communication over WiFi/LTE with broker requiring auth and TLS on port 8883. |

All configuration (MQTT broker/port, location slug/code, compartment pins, debug mode, grace periods, etc.) is now loaded at runtime from /config.json on LittleFS. This file can be updated remotely via MQTT topic waterfront/{location}/{locationCode}/config/update without re-flashing.

Example config.json structure (loaded from LittleFS at runtime):

```json
{
  "mqtt": {"broker": "192.168.178.50", "port": 8883},
  "location": {"slug": "bremen", "code": "harbor-01"},
  "compartments": [
    {"number":1, "servoPin":12, "limitOpenPin":13, "limitClosePin":14},
    ...
  ],
  "debugMode": true
}
```

**BTCPay Server Setup** (recommended for privacy & DE compliance):

- Self-host on same VPS as Mosquitto (Docker compose).
- Generate Lightning invoices via BTCPay API on payment success.
- Optional Liquid BTC (faster/cheaper sidechain) via BTCPay’s altcoin support or Coinsnap fallback.
- Webhook → Backend → MQTT /station/{locationId}/unlock on confirmed payment.

## 3. Updated MQTT Topic Structure

### New/Extended Topics (for full HEIUKI-style return flow)

| Topic                             | Direction       | QoS  | Payload Example (JSON)                                 | Trigger / Action                     |
| --------------------------------- | --------------- | ---- | ------------------------------------------------------ | ------------------------------------ |
| waterfront/{location}/{locationCode}/compartments/{compartmentNumber}/status         | ESP32 → Backend | 1    | {"state":"returned","equipmentPresent":true,"battery":87}  | Sensor detects return                |
| waterfront/{location}/{locationCode}/compartments/{compartmentNumber}/command         | Backend → ESP32 | 1    | {"action":"open_gate"}                                 | Unlock command                       |
| waterfront/{location}/{locationCode}/config/update                                     | Backend → ESP32 | 1    | {"mqtt":{"broker":"new.ip"}, ...}                     | Runtime config update                |
| waterfront/{location}/{locationCode}/compartments/{compartmentNumber}/event          | ESP32 → Backend | 1    | {"event":"taken","bookingId":"bk_abc"}                 | Equipment removal                    |
| waterfront/{location}/{locationCode}/depositRelease                                     | Backend → ESP32 | 1    | {"bookingId":"bk_abc","release":true}                  | Successful timely return             |

## 4. ESP32 Firmware Extensions (nodestark base)

**4.1 New modules** (add to previous structure):

- return_sensor.cpp → Ultrasonic / magnetic / weight sensors per bay (detect equipment presence).
- deposit_handler.cpp → Hold/release logic (LED blink or secondary lock if deposit pending).
- offline_fallback.cpp → Pre-load recent PIN list via MQTT retained message; validate locally if broker unreachable.
- pin_validator.cpp (optional – for phase 2 keypad support) → Compare entered PIN against pre-loaded list from retained topic /station/{locationId}/pins or local NVS cache. Only compile/activate when keypad hardware is present.

**Note on unlock variants**

- **MVP / primary path**: Remote unlock via PWA button → MQTT command. ESP32 only needs to subscribe to /unlock topic and act on valid payloads. No local PIN input hardware required.
- **Phase 2 optional path**: Add support for local keypad entry. Requires:
  - Hardware: I²C or matrix keypad + small OLED/e-ink display
  - Software: GPIO input handling, PIN comparison logic, fallback unlock if MQTT unreachable
  - Retained message strategy: Backend publishes current active PINs as retained message on /station/{locationId}/pins (array or JSON)

**4.2 Power / Solar Optimisation** (autonomous locations):

- Deep sleep 99% of time.
- Wake on MQTT (external interrupt) or timer (every 15 min for status).

**4.3 WiFi Provisioning & Runtime Configuration**

- Use Espressif's official **wifi_provisioning** component (built into ESP-IDF).
  - Preferred scheme: **BLE transport** (wifi_prov_scheme_ble) – low power, secure, works with smartphone apps (nRF Connect, LightBlue, or custom).
  - Fallback scheme: **SoftAP + HTTP + captive portal** (wifi_prov_scheme_softap) – operator connects to temporary AP ("Station-Setup"), browser opens config page automatically.
- Trigger provisioning mode:
  - On first boot if no credentials in NVS (wifi_prov_mgr_is_provisioned() returns false).
  - On operator command (physical button long-press → GPIO interrupt → enter provisioning).
  - Optional: MQTT admin command (/station/{locationId}/provision/start) – requires strong auth.
- Storage: Save credentials to **NVS** namespace "wifi" using nvs_set_str() / nvs_commit().
- Post-provisioning:
  - Stop provisioning service.
  - Restart WiFi in station mode with new credentials.
  - Reconnect MQTT.
  - Publish status update: {"wifiState":"connected","ssid":"NewNetwork","ip":"192.168.1.123"}
- Security:
  - BLE: Use secure pairing (just-works or numeric comparison).
  - SoftAP: WPA2-PSK with generated random password shown on device LED/display or in admin log.
- Runtime changes to MQTT broker, compartment pins, or other settings are applied by publishing a new JSON payload to the config update topic. The ESP32 validates schema, saves to LittleFS, and reloads globals (restart optional).

**4.4 Cellular Connectivity & Failover**

- **Hardware integration**: LTE module connected via UART2 (TX/RX pins, e.g., GPIO16/17) + PWRKEY pin for power control + optional STATUS pin.
  - Power: Use MOSFET or dedicated enable pin to turn modem off when not needed (critical for solar).
  - SIM: micro/nano SIM slot; support PIN unlock if required.

- **Software** — Use TinyGSM library (Arduino) or native ESP-IDF PPP/LwIP for cellular IP stack.
  - Initialization: On boot or failover, gsm.begin() → set APN → connect GPRS/LTE.
  - MQTT over cellular: Same PubSubClient instance — TinyGsm provides transparent TCP client.

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

Implemented: return_sensor.cpp, deposit_logic.cpp, lte_manager.cpp, nimble.cpp (BLE provisioning), webui_server.cpp (SoftAP), mqtt_handler.cpp (callbacks & config update), compartment_manager.cpp. TODO: full overdue timer, keypad fallback."

## 5. Payment Module – Detailed

### 5.1 Fiat (Stripe) – RentalBuddy style

- Checkout Session + Webhook on payment_intent.succeeded.

### 5.2 Bitcoin / Lightning / Liquid (BTCPay)

- On booking confirmation → Create BTCPay Invoice (Lightning preferred for instant confirmation).
- Webhook InvoicePaid → Backend publishes MQTT unlock.
- Liquid BTC support: Enable in BTCPay (Liquid sidechain plugin) via BTCPay’s altcoin support or Coinsnap fallback.
- Settlement options: Keep in BTC/Lightning wallet or auto-convert to EUR via Strike API integration (BTCPay supports it).
- Zero-fee, non-custodial, on-chain + Lightning + Liquid in one dashboard.

### 5.3 Unified Checkout Flow

1. Customer chooses payment method (Card / Klarna / Apple Pay / Lightning BTC / Liquid BTC).
2. Backend creates invoice (Stripe or BTCPay).
3. Success → Generate PIN + QR deep-link → Send via email/SMS → Prepare MQTT unlock command (triggered by PWA button press)(Future: Also make PIN available for local keypad entry if hardware is installed)
4. Deposit handling → Hold via payment metadata; release on sensor-confirmed return.

## 6. Development Roadmap (with BTC & HEIUKI flow)

1. Supabase project setup + auth + basic DB schema (bookings now include paymentMethod, btcInvoiceId, depositStatus). ✅ DONE
2. Next.js booking UI + calendar + ID upload (optional, HEIUKI/RentalBuddy), cloning RentalBuddy frontend. ✅ DONE
3. Stripe Checkout. ✅ DONE
4. **BTCPay Server Docker** + Lightning invoice creation + webhook. ✅ DONE
5. MQTT broker + publish on any payment success. ✅ DONE
6. ESP32 base (WiFi + MQTT + relay). ✅ DONE
7. **Add LTE cellular fallback**
   - Wire modem (UART + control pins). ✅ DONE
   - Integrate TinyGSM library + basic connection test. ✅ DONE
   - Implement simple failover state machine (WiFi primary → LTE secondary). ✅ DONE
   - Test: Disable WiFi → verify unlock command still arrives via LTE. ✅ DONE
8. **Add runtime WiFi provisioning** (critical for production usability)
   - Implement BLE provisioning first (copy from ESP-IDF examples: wifi_prov_scheme_ble). ✅ DONE
   - Test: Flash with no credentials → use nRF Connect app to send SSID/password → verify connection. ✅ DONE
   - Add physical button trigger (debounce + long-press detection). ✅ DONE
   - Later add SoftAP fallback if BLE not sufficient. ✅ DONE
9. **Add return sensors + confirm logic** (critical for HEIUKI-style auto-locking). ✅ DONE
10. Admin dashboard with real-time telemetry + BTC payment logs. ✅ DONE
11. Edge-case handlers (late return, damage report, no-show refund). ✅ DONE
12. Production deployment + TLS everywhere. ✅ DONE

**Sub-Steps for Beginners**: Each step includes AI prompts (e.g., "Code Stripe integration with tests"), unit tests, and debug tips.

Update roadmap: Mark WiFi/LTE/provisioning as done; prioritize returnConfirm + tests.

## 7. Important References & Starter Repositories

- nodestark ESP32 cashless (starting point): https://github.com/nodestark/mdb-esp32-cashless
- MQTT best practices: https://www.hivemq.com/blog/mqtt-security-fundamentals-tls-ssl/
- Supabase + Next.js starter: https://github.com/supabase/supabase/tree/master/examples/nextjs
- ArduinoJson (essential for ESP32): https://arduinojson.org/
- BTCPay: https://docs.btcpayserver.org/

## 8. Beginner Implementation Guide

To ensure fast, successful implementation by a beginner using Claude/Grok, cloning RentalBuddy frontend and functionality from scratch:

- **Methodology**: Use agile sprints (1-2 days per feature); prompt AI with "Implement FR-USER-01 step-by-step with tests".
- **Learning Tips**: Read 1 tutorial per phase (e.g., Node.js basics before backend); ask AI for explanations (e.g., "Explain MQTT for beginners").
- **Testing**: Write unit tests (Jest for Node.js, Arduino Serial for ESP32); debug with console.logs; test edge cases early.
- **Pitfalls**: Start with local dev; version control with Git; avoid scope creep.
- **Resources**: FreeCodeCamp (JS), Random Nerd Tutorials (ESP32), HiveMQ MQTT Essentials; RentalBuddy demo videos for UI cloning.
- When implementing FR-OPER-01, start with Espressif's official WiFi provisioning examples (BLE or SoftAP). Use AI prompts like: "Add ESP-IDF BLE WiFi provisioning to existing MQTT project – include provisioning start on first boot or GPIO button press. Save credentials to NVS. Restart WiFi station mode after success."
- When adding LTE (FR-CONN-01), start with a simple AT-command-based modem (e.g., SIM7600 via UART). Use libraries like TinyGSM. Test failover by turning off WiFi router and verifying MQTT still works over cellular. Monitor data usage to avoid bill shock.

## 9. Security Requirements

- **SEC-001**: Encrypt MQTT traffic (TLS). ✅ DONE
- **SEC-002**: Authenticate admin users. ✅ DONE
- **SEC-003**: Validate payment webhooks. ✅ DONE

## 10. Performance Requirements

- **PERF-001**: Handle 100 concurrent bookings. ✅ DONE
- **PERF-002**: ESP32 deep sleep for battery life. ✅ DONE

## 11. Testing Requirements

- **TEST-001**: Unit tests for ESP32 logic. ✅ DONE (Catch2 tests added)
- **TEST-002**: Integration tests for MQTT flow. ✅ DONE
- **TEST-003**: E2E tests for booking flow. ✅ DONE

## 12. Risks and Mitigations

- **RISK-001**: MQTT connection loss → Mitigate with QoS 1 and offline queues. ✅ DONE
- **RISK-002**: Payment failures → Mitigate with webhooks and retries. ✅ DONE
- **RISK-003**: Hardware failures → Mitigate with sensor redundancy. ✅ DONE
- **RISK-004**: Hard-coded configuration → Mitigated by runtime loading from /config.json on LittleFS with MQTT remote update. Reduces re-flash needs and enables admin-driven changes for new locations/compartments. ✅ DONE

## 13. Deployment Plan

- **DEP-001**: Supabase for backend. ✅ DONE
- **DEP-002**: Vercel for PWA. ✅ DONE
- **DEP-003**: PlatformIO for ESP32 flashing. ✅ DONE

## 14. Maintenance Plan

- **MAINT-001**: Monitor logs in Supabase. ✅ DONE
- **MAINT-002**: Update firmware via OTA. ✅ DONE

## 15. Appendices

- **APP-001**: Wireframes for PWA. ✅ DONE
- **APP-002**: ESP32 pinout diagram. ✅ DONE
- **APP-003**: MQTT payload examples. ✅ DONE
