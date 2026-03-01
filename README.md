# Waterfront – Self-Service Kayak & SUP Rental System

Unmanned, 24/7 kayak/SUP rental platform with **Next.js PWA**, **Supabase** backend, **Stripe + BTCPay** payments (fiat + Lightning/Liquid BTC), and **ESP32 MQTT** smart-locker control.

**Live demo (frontend only – Vercel)**: https://waterfront-[your-project-slug].vercel.app  
**Current status**: Early development – auth, local Supabase, calendar skeleton & MQTT base working. Payments & ESP32 sensor logic next.

## Core Concept

Users book online → pay → receive PIN/QR → arrive at solar-powered locker → enter code / scan → take kayak → return to same bay → sensors confirm → deposit released automatically.

## Features – Status

| Feature                               | Status    | Notes / Next                   |
| ------------------------------------- | --------- | ------------------------------ |
| Guest booking (no login required)     | ✅ Working | Calendar + time slots          |
| Real-time availability                | 🟡 Partial | Supabase Realtime              |
| Fiat payments (Stripe)                | 🚧 Planned | Checkout + webhook             |
| Lightning / Liquid BTC (BTCPay)       | 🚧 Planned | Webhook → MQTT unlock          |
| PIN/QR generation & delivery          | 🟡 Partial | Email / on-screen              |
| ESP32 MQTT locker control             | ✅ Base    | Unlock command, status publish |
| Kayak presence sensor (ultrasonic)    | 🚧 Planned | Taken / returned events        |
| Deposit handling & auto-release       | 🚧 Planned | MQTT + backend logic           |
| Admin dashboard (bookings, telemetry) | 🚧 Planned | Supabase + real-time           |
| Offline-tolerant PWA                  | 🟡 Partial | QR/PIN caching                 |

## WATERFRONT Documentation Index

##### Core Specifications
- [Technical Specification Document (TSD)](Technical%20Specification%20Document%20(TSD).md) — full system overview, flows, BTC/Lightning, edge cases
- [Functional Specification Document (FSD)](Functional%20Specification%20Document%20(FSD).md) — requirements, architecture, data models

##### ESP32 Firmware Preparation
- [ESP32 Pinout Plan and Diagram](ESP32%20Pinout%20Plan%20and%20Diagram.md) — GPIO assignments + wiring
- [ESP32 Project Plan – Nodestark Adaptation](ESP32%20Project%20Plan%20-%20Waterfront%20Firmware%20Code%20Development.md) — step-by-step adaptation roadmap
- [ESP32 Firmware – Test Checklist](ESP32%20Firmware%20-%20Test%20Checklist.md) — phased verification

##### Integration & Architecture
- [HARDWARE_BASELINE](HARDWARE_BASELINE.md) — confirmed board, sensors, modem, power
- [MQTT_SPEC](MQTT_SPEC.md) — topics, payloads, QoS
- [REST-MQTT-Architecture](REST-MQTT-Architecture.md) — backend → MQTT → ESP32 flow

##### UI/UX Specs
- [APP Admin - Layout Spec](APP%20Admin%20-%20Layout%20Spec.md)
- [APP Booking - PWA Layout Spec](APP%20Booking%20-%20PWA%20Layout%20Spec.md)

## Tech Stack

- **Frontend / PWA** — Next.js 15+, TypeScript, Tailwind CSS, shadcn/ui, Vercel deployment
- **Backend / Database / Auth** — Supabase (PostgreSQL + Auth + Realtime + Storage)
- **Payments** — Stripe Checkout + BTCPay Server (self-hosted, Lightning/Liquid)
- **IoT Controller** — ESP32-S3 (ESP-IDF), MQTT (PubSubClient or esp-mqtt), TinyGSM (LTE fallback)
- **MQTT Broker** — Mosquitto (Docker)
- **Development** — pnpm, Aider (AI pair programming), PlatformIO, GitHub monorepo
- **Deployment** — Vercel (frontend), Railway/Render/VPS (MQTT broker, future BTCPay)

## Project Structure (Monorepo)

Waterfront/ 

├── docs/                   # Functional/Technical specs, wireframes, schemas 

├── supabase-local/         # Local Supabase dev (CLI + migrations) 

├── waterfront-web/         # Next.js PWA → deployed to Vercel 

├── waterfront-esp32/       # ESP-IDF project (PlatformIO/VS Code) 

├── waterfront-infra/       # Docker Compose (Mosquitto, future BTCPay) 

└── README.md

## Quick Start – Local Development

### Prerequisites

- Node.js 20+ (nvm recommended)
- pnpm (`npm install -g pnpm`)
- Docker Desktop
- Supabase CLI (`brew install supabase/tap/supabase` or `npm install -g supabase`)
- PlatformIO (VS Code extension)
- Git

### 1. Clone & Install

```bash
git clone https://github.com/[your-username]/Waterfront.git
cd Waterfront

cd waterfront-web
pnpm install
```

### 2. Run the PWA

Bash

```
cd waterfront-web
pnpm dev
```

→ [http://localhost:3000](http://localhost:3000/)

### 3. Start Local Supabase

Bash

```
cd ../supabase-local
supabase init     # only first time
supabase start
```

→ Studio: [http://127.0.0.1:54323](http://127.0.0.1:54323/) Login: postgres / postgres

### 4. Start Local MQTT Broker

Bash

```
cd ../waterfront-infra
docker compose up -d
```

→ mqtt://localhost:1883 (anonymous)

### 5. Build & Flash ESP32 Firmware

The ESP32 controller uses **PlatformIO + ESP-IDF framework** (not Arduino core).

#### Prerequisites
- Install **VS Code** + **PlatformIO IDE** extension
- (Optional but recommended) Also install **ESP-IDF** extension by Espressif for menuconfig & advanced debugging

#### Steps
1. Clone the repo:
   ```bash
   git clone https://github.com/BBXtreme/Waterfront.git
   cd Waterfront/waterfront-esp32



##### Build / Upload / Monitor:

- **Build**: Click PlatformIO → Build (checkmark icon) or run pio run in terminal

- **Upload / Flash**: Click PlatformIO → Upload (right arrow icon) or pio run -t upload

- **Serial Monitor**: Click PlatformIO → Serial Monitor (plug icon) or pio device monitor

  

## Environment Variables

### waterfront-web/.env.local (local dev)

env

```
# Supabase local (MUST use port 54321) 
NEXT_PUBLIC_SUPABASE_URL=http://127.0.0.1:54321 NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9... 

# MQTT broker (local) 
MQTT_BROKER_URL=mqtt://localhost:1883 

# Payments (add later) 
NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_test_... STRIPE_SECRET_KEY=sk_test_... BTCPAY_URL=https://your-btcpay-server.com BTCPAY_API_KEY=...
```

**Rule**: NEXT_PUBLIC_ → client-side only. Server secrets stay out of git.

## Useful Commands

Bash

```
# PWA dev server
cd waterfront-web && pnpm dev

# Local Supabase
cd supabase-local && supabase start

# MQTT broker
cd waterfront-infra && docker compose up -d

# Stop everything
cd waterfront-infra && docker compose down

# Open entire repo in VS Code
code .
```

## Development Guidelines

- Use **Aider** for code generation/refactoring:

  Bash

  ```
  aider --model xai/grok-code-fast-1
  ```

- Small, focused commits

- Branch naming: feat/calendar-availability, fix/mqtt-reconnect, refactor/remove-mdb

- Document schemas/decisions in docs/

- Keep waterfront-web as priority until booking/payment flow is complete

## License

MIT License (to be confirmed)

Questions, ideas or help needed? Open an Issue or DM on X @BangLee8888.

Happy building!
