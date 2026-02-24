- # Functional Specification Document (FSD)

  ## Document Information

  - **Document Title**: WATERFRONT -> Functional Specification for Kayak Rental Booking App and ESP32-Cashless Integration
  - **Version**: 1.2 (Updated for Scratch Implementation Cloning RentalBuddy)
  - **Date**: February 23, 2026
  - **Author**: Grok AI (Assisted by xAI)
  - **Prepared For**: BangLee (@BangLee8888), SuperGrok User, Bremen, DE
  - **Purpose**: This FSD outlines the functional and non-functional requirements for developing a self-service kayak rental booking application (cloned from RentalBuddy-like features) integrated with an ESP32-based cashless controller for vending machines. It serves as a blueprint for step-by-step coding using tools like Claude and Grok, ensuring modular development and seamless MQTT communication for remote lock control and telemetry. Updates in 1.2 focus on implementing from scratch using Supabase + Next.js to closely clone RentalBuddy frontend and functionality (e.g., 24/7 unmanned rentals, Stripe integration, digital lock/PIN access, ID checks, website embedding, reminders), with beginner-friendly guidance for fast, successful realization through gradual coding, testing, debugging, and on-the-job learning.

  ## Revision History

  | Version | Date         | Description                                                  | Author  |
  | ------- | ------------ | ------------------------------------------------------------ | ------- |
  | 1.0     | Feb 22, 2026 | Initial Draft                                                | Grok AI |
  | 1.1     | Feb 22, 2026 | Added beginner guides, detailed roadmaps, testing strategies | Grok AI |
  | 1.2     | Feb 23, 2026 | Revised for scratch implementation cloning RentalBuddy frontend and functionality; removed bookcars references | Grok AI |

  ## Table of Contents

  1. Introduction
  2. Scope
  3. System Overview
  4. Functional Requirements
  5. Non-Functional Requirements
  6. System Architecture
  7. Data Models
  8. User Interfaces
  9. Integration and APIs
  10. Assumptions and Dependencies
  11. Risks and Mitigations
  12. Glossary
  13. Appendices
  14. Beginner Developer Guide (Updated)

  ## 1. Introduction

  ### 1.1 Project Background

  The project aims to create a web/mobile booking application for self-service kayak rentals, closely cloning RentalBuddy, allowing users in locations like Bremen, DE, to book, pay, and access kayaks via unmanned vending machines. The app will integrate with custom ESP32 controllers (based on nodestark/mdb-esp32-cashless open-source code) using MQTT for real-time communication, enabling remote unlocking of kayak bays, telemetry (e.g., battery status, sensor data), and secure access via PIN/QR codes. This supports solar/battery-powered off-grid operations, ideal for waterfront vending setups.

  ### 1.2 Objectives

  - Provide 24/7 online booking with time slots, payments, and automated access codes.
  - Ensure secure, low-latency communication between the app and ESP32 controllers for lock control.
  - Support admin features for inventory management, reports, and reminders.
  - Enable scalable deployment for multiple vending machines.
  - Facilitate beginner-friendly development with step-by-step guidance for AI-assisted coding, cloning RentalBuddy frontend (e.g., calendar, checkout, PIN display) and functionality (e.g., unmanned flow, ID verification, reminders).

  ### 1.3 Stakeholders

  - **End Users**: Customers booking kayaks (e.g., tourists at water sites).
  - **Operators/Admins**: Business owners managing fleets, stats, and maintenance.
  - **Developers**: Amateur programmers using this FSD for iterative coding with AI tools like Claude and Grok.

  ## 2. Scope

  ### 2.1 In Scope

  - User registration/login, booking calendar, payment processing (Stripe/Klarna/Apple Pay, BTC/Lightning/Liquid).
  - Generation of PIN/QR codes for access.
  - MQTT-based communication for unlock commands, telemetry feedback.
  - ESP32 adaptations: Lock relay control, sensor integration (e.g., presence detection), power monitoring.
  - Admin dashboard for reports, reminders, and machine status.
  - Basic security (TLS, JWT) and offline tolerance.
  - Beginner guides for incremental implementation cloning RentalBuddy.
  - Support for **dual connectivity** (WiFi primary + LTE cellular fallback) on the ESP32 controller to ensure reliable MQTT communication in areas with poor or no Wi-Fi coverage.
  - Operator shall be able to monitor current connectivity type (WiFi vs. LTE) and signal strength via admin dashboard telemetry.

  ### 2.2 Out of Scope

  - Hardware manufacturing (e.g., custom PCBs beyond ESP32 adaptations).
  - Advanced AI features (e.g., predictive availability).
  - Multi-language support (default: English/German).
  - Physical vending machine construction.

  ## 3. System Overview

  The system consists of:

  - **Booking App**: Web/PWA (Next.js) and optional mobile (React Native) for user bookings, cloning RentalBuddy's self-service interface.
  - **Backend Server**: Node.js/Express or Next.js API routes for logic, database (PostgreSQL/Supabase), payments, and MQTT publishing.
  - **MQTT Broker**: Mosquitto/EMQX for real-time messaging.
  - **ESP32 Controller**: Adapted from nodestark/mdb-esp32-cashless; handles locks, sensors, and MQTT subscriptions.
  - **Flow**: User books → Pays → Backend publishes MQTT unlock → ESP32 unlocks lock → Publishes status back.

  ## 4. Functional Requirements

  ### 4.1 User Module

  - FR-USER-01: Users can register/login via email/SMS (Supabase auth).
  - FR-USER-02: View available kayaks/time slots via calendar (filter by location, e.g., Bremen harbors).
  - FR-USER-03: Select kayak, time duration (e.g., 1-4 hours), add-ons (life jackets), confirm Terms&Conditions etc. and proceed to checkout.
  - FR-USER-04: Complete payment (advance + deposit) via Stripe or BTCPay (BTC/Lightning/Liquid); generate unique PIN/QR code.
  - FR-USER-05: Receive booking confirmation/reminders via email/SMS.
  - FR-USER-06: View booking history and cancel (with refunds per policy).
  - **FR-USER-07: Access & Unlock Mechanism** The system shall provide a secure, internet-dependent unlock method via the Progressive Web App (PWA).
    - Primary unlock method (solution of choice):
      - After successful payment and booking confirmation, the user receives a personalized deep-link (e.g. https://app.waterfront.rent/access?bid=abc123&token=xyz789) via email/SMS and/or QR code.
      - Opening the link loads a dedicated access screen in the browser/PWA showing:
        - Booking summary (machine, time window, remaining time)
        - Large prominent **"Unlock Now"** button
        - Optional countdown until rental start / auto-disable outside window
      - Tapping **"Unlock Now"** (after optional client-side time/window validation) triggers a secure API call to the backend.
      - Backend validates authorization, booking status, time window, and publishes MQTT unlock command.
    - Secondary / future method (optional phase 2+):
      - Physical numeric keypad + small display or e-ink screen mounted on the vending machine.
      - User manually enters the 4–6 digit PIN received in confirmation.
      - ESP32 locally validates PIN against pre-synchronized list (retained MQTT message or small local cache) → unlocks if valid and within time window.
      - This method provides offline fallback capability but is **not required for MVP**.

  ### 4.2 Admin Module

  - FR-ADMIN-01: Dashboard to manage kayak inventory (add/edit machines, locations, availability).
  - FR-ADMIN-02: View real-time telemetry (battery, sensors, lock status) from ESP32 via MQTT.
  - FR-ADMIN-03: Generate reports (bookings, revenue, usage stats).
  - FR-ADMIN-04: Send manual reminders or override unlocks.

  ### 4.3 Payment Module

  - FR-PAY-01: Integrate Stripe for secure fiat payments (cards, Klarna, Apple Pay).
  - FR-PAY-02: Integrate BTCPay for BTC/Lightning/Liquid payments (non-custodial, instant invoices).
  - FR-PAY-03: Handle deposits/refunds (e.g., auto-refund on timely return via sensor feedback).
  - FR-PAY-04: Support promotional codes/discounts.
  
  ### 4.4 MQTT Communication Module
  
  - FR-MQTT-01: Backend publishes unlock commands (JSON: {bookingId, pin, duration}) to topic /kayak/<machineId>/unlock.
  - FR-MQTT-02: ESP32 subscribes to unlock topics; validates PIN and triggers lock relay.
  - FR-MQTT-03: ESP32 publishes status (e.g., {status: "opened", battery: 80%}) to /kayak/<machineId>/status.
  - FR-MQTT-04: Support QoS 1 for reliable delivery; queue messages if offline.
  
  ### 4.5 ESP32 Controller Module
  
  - FR-ESP-01: Adapt nodestark code to handle MQTT (PubSubClient lib).
  - FR-ESP-02: Control relay for 12V solenoid locks (pulse open/close).
  - FR-ESP-03: Integrate sensors (e.g., ultrasonic for kayak presence, ADC for battery voltage).
  - FR-ESP-04: Deep sleep mode for power efficiency; wake on MQTT or timer.
  - FR-ESP-05: Optional MDB for cashless payments (if hybrid coin/card needed).
  
  ### 4.6 Operator Management Module
  
  - FR-OPER-01: The operator shall be able to change the WiFi network credentials (SSID and password) of a deployed vending machine **during normal operation** without physical access to re-flash firmware.
    - Acceptance criteria:
      - Change shall be possible via a secure, user-friendly method (Bluetooth LE provisioning preferred; SoftAP + captive portal as fallback).
      - New credentials shall be persistently stored (NVS) and applied after restart.
      - Machine shall automatically attempt to connect to the new network and resume MQTT operation.
      - If connection fails after change, machine shall fall back to provisioning mode or a safe default state (e.g., status LED pattern indicating error).
  - FR-OPER-02: Optional physical trigger (e.g., long-press button on the machine) to enter provisioning/configuration mode.
  - FR-OPER-03: Operator shall receive confirmation (via MQTT status publish or LED pattern) when new WiFi is successfully connected.
  
  ### 4.7 Connectivity Management

  - FR-CONN-01: The vending machine controller shall support LTE cellular connectivity as a fallback or alternative to WiFi.
    - The system shall automatically switch to LTE when WiFi is unavailable or signal is too weak (RSSI threshold configurable).
    - MQTT communication shall remain functional over LTE (using the same broker/topics).
    - Telemetry shall include current connection type (WiFi/LTE), signal strength (RSSI), and data usage estimate (to monitor SIM plan).
  - FR-CONN-02: Operator shall be able to configure LTE APN, SIM PIN (if required), and preferred mode (WiFi only / LTE only / auto-failover) via runtime provisioning (same mechanisms as WiFi config: BLE or MQTT).

  ## 5. Non-Functional Requirements

  ### 5.1 Performance
  
  - Response time: <2s for bookings; <1s for MQTT commands.
  - Scalability: Handle 100 concurrent users; 10+ machines.
  - Connectivity switchover (WiFi ↔ LTE) shall occur within 60 seconds of detecting failure, with no loss of queued MQTT messages.
  
  ### 5.2 Security

  - Data encryption: TLS for MQTT/API; JWT for auth.
  - Compliance: GDPR for user data (DE-based).
  - Access control: Role-based (user/admin); PIN expiration after rental.
  
  ### 5.3 Reliability
  
  - Uptime: 99% for backend/broker.
  - Offline handling: ESP32 queues telemetry; app shows last-known status.

  ### 5.4 Usability

  - Mobile-responsive UI (PWA for web embedding like RentalBuddy).
  - Accessibility: WCAG 2.1 compliant.
  
  ### 5.5 Maintainability
  
  - Modular code: Separate concerns (e.g., booking logic vs. MQTT).
  - Documentation: Inline comments; use this FSD for coding steps.
  
  ## 6. System Architecture
  
  - **High-Level Diagram** (Textual Representation):
  
    text
  
    ```
    User Device (Web/Mobile App) <--> Backend Server (Node.js + Supabase DB) <--> MQTT Broker (Mosquitto)
                                                                 |
                                                                 v
    ESP32 Controller (Adapted nodestark code) <--> Hardware (Locks, Sensors, Solar/Battery)
    ```
  
  - **Components**:
  
    - Frontend: Next.js for UI, cloning RentalBuddy's clean, self-service design.
    - Backend: Next.js API routes or Express.js for APIs; Supabase for auth/DB.
    - Integration: MQTT.js for publishing; PubSubClient on ESP32.
  
  - **Deployment**: Vercel for frontend; Render for backend; VPS for broker.
  
  ## 7. Data Models
  
  ### 7.1 Key Entities (PostgreSQL Schema)
  
  - **User**: id (PK), email, name, role (user/admin).
  - **Booking**: id (PK), userId (FK), machineId (FK), startTime, endTime, status (pending/confirmed/completed), pinCode, paymentId.
  - **Machine**: id (PK), location (e.g., "Bremen Harbor"), kayaksAvailable, status (online/offline), batteryLevel.
  - **Payment**: id (PK), bookingId (FK), amount, status, transactionId (Stripe/BTCPay).

  ## 8. User Interfaces

  - **Screens/Wireframes** (Textual):
    - Home: Calendar view with available slots, cloned from RentalBuddy's intuitive layout.
    - Booking: Form for selection, add-ons, checkout.
    - Confirmation: PIN/QR display, map to machine.
    - Admin: Dashboard with charts, machine list, telemetry feeds.
  - **Access / Unlock Screen** (mobile-first, PWA):
    - Full-screen layout optimized for smartphone
    - Header: "Your Kayak is Ready – Bremen Harbor Bay 1"
    - Large centered QR code (scannable fallback)
    - Below QR: 6-digit PIN in bold monospace font
    - Prominent **green "Unlock Now"** button (disabled + reason shown if outside time window or already used)
    - Status area: real-time feedback after tap ("Unlocking…", "Opened!", "Failed – try again")
    - Small text: "Ensure you are at the machine. Internet connection required for this action."
    - Optional future variant: "Alternatively enter PIN on machine keypad" hint

  ## 9. Integration and APIs
  
  - **External**: Stripe/BTCPay for payments; Twilio for SMS.
  - **Internal**: RESTful APIs (e.g., POST /bookings); MQTT topics as defined.
  - **Error Handling**: Retry failed MQTT publishes; log errors.

  ## 10. Assumptions and Dependencies

  - Assumptions: Users have internet; ESP32 has Wi-Fi or LTE cellular access.
  - Dependencies: Open-source repos (nodestark for ESP32); Stripe/BTCPay accounts; VPS hosting; Supabase + Next.js starters for scratch implementation.
  
  ## 11. Risks and Mitigations
  
  - Risk: MQTT latency → Mitigation: Use QoS 2; fallback to local PIN entry.
  - Risk: Battery drain → Mitigation: Deep sleep in ESP32 code.
  - Risk: Security breach → Mitigation: Regular audits; encrypted payloads.
  - Risk: Beginner errors → Mitigation: Step-by-step roadmaps, debugging tips (see Section 14).
  - Risk: WiFi network change required (router replacement, relocation, password update) → Machine becomes unreachable until re-flashed. Mitigation: Implement runtime WiFi provisioning (Bluetooth or SoftAP) as per FR-OPER-01. Provide clear operator instructions in admin dashboard / documentation.
  - Risk: LTE modem increases power consumption → shortens solar/battery runtime. Mitigation: Use low-power LTE-M or NB-IoT modes when available; implement aggressive power management (deep sleep, modem power-down when idle); monitor data usage to avoid bill shock.
  - Risk: Cellular data costs accumulate. Mitigation: Telemetry shall report monthly data usage; admin alerts if thresholds exceeded; prefer WiFi whenever possible.

  ## 12. Glossary

  - **MQTT**: Message Queuing Telemetry Transport—protocol for IoT comms.
  - **ESP32**: Microcontroller for vending control.
  - **PIN/QR**: Access codes for on-site verification.
  - **Telemetry**: Real-time data from ESP32 (e.g., status).
  - **BTCPay**: Open-source Bitcoin payment processor for Lightning/Liquid.
  
  ## 13. Appendices
  
  - Appendix A: Sample MQTT Payloads
    - Unlock: {"bookingId": "abc123", "pin": "4567", "duration": 7200}
    - Status: {"status": "opened", "battery": 85, "kayakPresent": true}
  - Appendix B: Coding Roadmap
    - Step 1: Set up Supabase + Next.js from scratch.
    - Step 2: Implement backend APIs and DB.
    - Step 3: Build frontend UI cloning RentalBuddy.
    - Step 4: Integrate MQTT in backend/ESP32.
    - Step 5: Test end-to-end flow.
    - Step 6: Deploy and monitor.
  
  ## 14. Beginner Developer Guide (Updated)
  
  To ensure fast, successful implementation by a beginner using Claude/Grok, cloning RentalBuddy frontend and functionality from scratch:
  
  - **Methodology**: Use agile sprints (1-2 days per feature); prompt AI with "Implement FR-USER-01 step-by-step with tests".
  - **Learning Tips**: Read 1 tutorial per phase (e.g., Node.js basics before backend); ask AI for explanations (e.g., "Explain MQTT for beginners").
  - **Testing**: Write unit tests (Jest for Node.js, Arduino Serial for ESP32); debug with console.logs; test edge cases early.
  - **Pitfalls**: Start with local dev; version control with Git; avoid scope creep.
  - **Resources**: FreeCodeCamp (JS), Random Nerd Tutorials (ESP32), HiveMQ MQTT Essentials; RentalBuddy demo videos for UI cloning.
  - When implementing FR-OPER-01, start with Espressif's official WiFi provisioning examples (BLE or SoftAP). Use AI prompts like: "Add ESP-IDF BLE WiFi provisioning to existing MQTT project – include GATT service for SSID/password write, store in NVS, restart WiFi on success."
  - When adding LTE (FR-CONN-01), start with a simple AT-command-based modem (e.g., SIM7600 via UART). Use libraries like TinyGSM. Test failover by turning off WiFi router and verifying MQTT still works over cellular.