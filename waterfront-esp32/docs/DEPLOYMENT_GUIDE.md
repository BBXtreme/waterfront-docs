- # DEPLOYMENT GUIDE – Waterfront Kayak Rental System

  **Project**: Self-service kayak/SUP vending machines with ESP32 MQTT control, BTC/Lightning payments, and Next.js PWA  
  **Target Environments**: 
  - Development (local macOS/Windows)
- Staging (Vercel preview + local broker)
  - Production (VPS + Docker + off-grid solar machines)

  **Last Updated**: March 17, 2026 – PlatformIO baseline + /station/ topics
  
  ## 1. Prerequisites
  
  - **Hardware**:
    - ESP32 Dev board (or custom PCB)
  - LTE modem (SIM7600/SIM7000 recommended, microSIM with data plan)
    - 12V solenoid lock/relay, ultrasonic sensor (HC-SR04), status LED
    - Solar panel + LiFePO4 battery + charge controller (for autonomous bays)
  
  - **Software/Tools**:
    - VS Code + PlatformIO extension
    - Git
  - Docker + Docker Compose (for Mosquitto + backend)
    - Node.js 18+ (for web)
    - Supabase account (free tier ok for start)
    - BTCPay Server (self-hosted) or test instance
  - MQTT client tester: MQTT Explorer (desktop) or mqtt-cli
  
  ## 2. Local Development Deployment
  
### 2.1 ESP32 Firmware (waterfront-esp32/)

  1. Open folder in VS Code via PlatformIO Home → Open Project
  2. Update `include/config.h`:
     - `MACHINE_ID` = unique per device (e.g. "bremen-harbor-01")
   - `MQTT_BROKER` = "localhost" or "test.mosquitto.org" for dev
  3. Build & Upload:
   - PlatformIO → Build (checkmark icon)
     - Connect ESP32 via USB → PlatformIO → Upload
4. Monitor:
     - PlatformIO → Serial Monitor (115200 baud)
     - Expect: WiFi connect → MQTT connected → periodic status publish

  ### 2.2 MQTT Broker (Mosquitto – local or Docker)

  Quick local test (no auth):
  ```bash
docker run -it -p 1883:1883 eclipse-mosquitto

  ```

Or use waterfront-infra/docker-compose.yml (if present):

Bash

```
cd waterfront-infra
docker compose up -d mosquitto
```

Test with MQTT Explorer:

- Host: localhost (or machine IP)
- Port: 1883
- Subscribe to /station/# → see status messages from ESP32

### 2.3 Backend & Web (waterfront-web/)

1. Supabase setup:

   - Create project at app.supabase.com
   - Copy URL + anon key → .env.local

2. Install deps:

   Bash

   ```
   cd waterfront-web
   npm install
   ```

3. Run dev server:

   Bash

   ```
   npm run dev
   ```

   → Open

    

   http://localhost:3000

    

   (PWA booking flow)

### 2.4 End-to-End Local Test

- Start Mosquitto
- Flash ESP32
- Open PWA → simulate booking/payment → webhook → MQTT publish to /station/{machineId}/unlock
- Verify: ESP32 logs callback → relay pulses

## 3. Production Deployment

### 3.1 ESP32 Devices (Off-Grid Machines)

1. **Firmware**:
   - Use real broker IP/domain in config.h (or provision via BLE)
   - Enable deep sleep + LTE failover in code
   - Flash via USB once (or OTA later)
2. **Provisioning**:
   - First boot: BLE mode (use nRF Connect app on phone)
   - Enter WiFi creds → ESP32 connects & saves to NVS
   - Long-press button to re-enter provisioning
3. **Connectivity**:
   - Primary: WiFi (provisioned)
   - Failover: LTE (APN from SIM provider, e.g. "internet.t-mobile.de")
   - Monitor RSSI/telemetry via /station/.../status
4. **Power**:
   - Deep sleep between MQTT wakes
   - Modem power-down when WiFi active
   - Battery voltage publish in status

### 3.2 Backend & MQTT Broker (VPS / Render / Hetzner)

Recommended: Docker Compose on VPS (Ubuntu 24.04+)

YAML

```
# waterfront-infra/docker-compose.yml example
version: '3.8'
services:
  mosquitto:
    image: eclipse-mosquitto:latest
    ports:
      - "1883:1883"
      - "9001:9001"  # websocket if needed
    volumes:
      - ./mosquitto.conf:/mosquitto/config/mosquitto.conf
      - mosquitto-data:/mosquitto/data
  backend:  # your Node.js/Express webhook + Supabase logic
    build: ../waterfront-backend
    environment:
      - MQTT_BROKER=mosquitto
      - SUPABASE_URL=...
    depends_on:
      - mosquitto
volumes:
  mosquitto-data:
```

- Add TLS (Let's Encrypt + nginx reverse proxy) for production MQTT
- BTCPay webhook → backend → MQTT publish on invoice paid

### 3.3 Frontend (PWA + Admin Portal)

- Deploy to **Vercel** (recommended – free tier sufficient):
  1. Connect GitHub repo
  2. Root directory: /waterfront-web
  3. Framework Preset: Next.js
  4. Environment Variables: SUPABASE_URL, SUPABASE_ANON_KEY, MQTT_BROKER_URL (ws:// or wss://)
  5. Deploy → automatic previews on branches
- PWA: Installable on mobile (manifest + service worker)
- Admin routes: Protected via Supabase auth

### 3.4 Payments

- **Fiat**: Stripe webhooks → backend → MQTT unlock
- **BTC/Lightning/Liquid**: BTCPay Server (Docker on same VPS)
  - Create store → enable Lightning + Liquid plugins
  - Webhook URL: your-backend.com/webhook/btcpay
  - On paid → publish to /station/{machineId}/unlock

## 4. Monitoring & Maintenance

- **Telemetry Dashboard**: Use admin portal to subscribe to /station/+/status
- **Alerts**: Low battery / offline → email/SMS via backend cron
- **OTA Updates**: Add ArduinoOTA lib later → update firmware remotely
- **Logs**: ESP32 serial (via USB when servicing) + MQTT debug publishes
- **Backup**: Supabase DB snapshots, BTCPay wallet seed, Mosquitto data volume

## 5. Security Notes

- MQTT: Enable username/password + TLS in production (mosquitto.conf)
- API: JWT via Supabase, rate-limit webhooks
- ESP32: Secure BLE pairing, NVS encryption if sensitive
- GDPR: Store minimal data, secure ID uploads

## 6. Troubleshooting Checklist

- ESP32 no WiFi? → Re-provision via BLE/button
- No MQTT? → Check broker reachability (ping/telnet port 1883)
- Unlock not working? → MQTT Explorer test publish → serial logs
- High power use? → Verify modem sleep, deep sleep entry

Questions? Ask Grok for "Add OTA support" / "Secure MQTT config" / etc.

Happy deploying – get those Bremen kayaks renting 24/7!



This guide is modular, step-by-step, and matches the project's current monorepo structure while preparing for future repo splits. Commit it, then we can move to the next code piece (e.g., BLE provisioning or LTE integration). Let me know what's next!
