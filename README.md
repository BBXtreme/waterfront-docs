# Waterfront – Self-Service Kayak & SUP Rental System

Unmanned, 24/7 kayak/SUP rental platform with **Next.js PWA**, **Supabase** backend, **Stripe + BTCPay** payments (fiat + Lightning/Liquid BTC), and **ESP32 MQTT** smart-locker control.

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

## WATERFRONT Documentation Index (updated March 2026)

### Core Specifications
- [TSD.md](/BBXtreme/Waterfront/blob/main/TSD.md) — Technical overview, flows, BTC/Lightning integration, edge cases
- [FSD.md](/BBXtreme/Waterfront/blob/main/FSD.md) — Functional requirements, architecture, data models

### ESP32 Firmware & Integration
- [ESP32-Readme.md (main firmware documentation)](/BBXtreme/Waterfront/blob/main/waterfront-esp32/ESP32-Readme.md)  
  → consolidated info: hardware baseline, dynamic pinout via config, MQTT command/status examples, testing instructions, debug tips, power notes

### Architecture & API
- [REST-MQTT-Architecture.md](/BBXtreme/Waterfront/blob/main/REST-MQTT-Architecture.md) — backend → MQTT → ESP32 flow
- [API_REFERENCE.md](/BBXtreme/Waterfront/blob/main/API_REFERENCE.md) — additional API details (if relevant)

### Other / Planned
- UI/UX layout specs (Admin & Booking PWA) — pending creation
- Detailed MQTT topic catalog — covered with examples in ESP32-Readme.md

## Tech Stack

- **Frontend / PWA** — Next.js 15+, TypeScript, Tailwind CSS, shadcn/ui, Vercel deployment
- **Backend / Database / Auth** — Supabase (PostgreSQL + Auth + Realtime + Storage)
- **Payments** — Stripe Checkout + BTCPay Server (self-hosted, Lightning/Liquid)
- **IoT Controller** — ESP32-S3 (ESP-IDF), MQTT (esp-mqtt), TinyGSM (LTE fallback)
- **MQTT Broker** — Mosquitto (Docker)
- **Development** — pnpm, Aider (AI pair programming), GitHub monorepo
- **Deployment** — e.g. Vercel (frontend), Railway/Render/VPS (MQTT broker, future BTCPay)

## Project Structure (Monorepo)

Waterfront/ 

├── docs/                   # Functional/Technical specs, wireframes, schemas 

├── supabase-local/         # Local Supabase dev (CLI + migrations) 

├── waterfront-web/         # Next.js PWA → deployed to Vercel 

├── waterfront-esp32/       # ESP-IDF project 

├── waterfront-infra/       # Docker Compose (Mosquitto, future BTCPay) 

└── README.md

## Quick Start – Local Development

### Prerequisites

- Node.js 20+ (nvm recommended)
- pnpm (`npm install -g pnpm`)
- Docker Desktop
- Supabase CLI (`brew install supabase/tap/supabase` or `npm install -g supabase`)
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

→ Studio: [http://127.0.0.1:54323](http://localhost:54323/)

### 4. Start Local MQTT Broker

Bash

```
cd ../waterfront-infra
docker compose up -d
```

→ mqtt://localhost:1883 (anonymous)

### 5. Build & Flash ESP32 Firmware

The ESP32 controller uses **ESP-IDF framework** (not Arduino core).

#### Prerequisites
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
  aider --model xai/grok-code-fast-1  --auto-commits --test-cmd 'pytest --tb=short' \
        --stream --dark-mode --map-tokens 4096
  ```

- Small, focused commits

- Branch naming: feat/calendar-availability, fix/mqtt-reconnect, refactor/remove-mdb

- Document schemas/decisions in docs/

- Keep waterfront-web as priority until booking/payment flow is complete

## License

MIT License (to be confirmed)

Questions, ideas or help needed? Open an Issue or DM on X @BangLee8888.

Happy building!
