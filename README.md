# Waterfront

**Kayak / SUP Rental Booking App + ESP32 MQTT Controller**  

Self-service, unmanned rental system with Next.js PWA, Supabase backend, BTCPay/Stripe payments, and ESP32-based smart locker control via MQTT.

## Features (Current & Planned)

- Guest booking (no login required)
- Calendar + time slot selection
- Location / equipment selection
- Real-time availability (Supabase)
- Payments: Stripe (fiat) + BTCPay (Lightning / Liquid BTC)
- Instant PIN + QR code confirmation (via email or on-screen)
- ESP32 MQTT integration for locker unlock / sensor feedback (take/return events)
- Admin dashboard (telemetry, bookings, payments, logs)
- Offline-tolerant PWA (cached QR/PIN)

## Tech Stack

- **Frontend / PWA** — Next.js 16+, TypeScript, Tailwind CSS, shadcn/ui
- **Backend / Database** — Supabase (PostgreSQL + Auth + Realtime + Storage)
- **Payments** — Stripe + BTCPay Server (self-hosted)
- **IoT / Hardware** — ESP32 (Arduino framework + PlatformIO), PubSubClient (MQTT), TinyGSM (LTE fallback)
- **Infrastructure** — Docker + Mosquitto MQTT broker
- **Development** — pnpm, Aider (AI coding), GitHub

## Project Structure (Monorepo)

Waterfront/ 

├── docs/                        # Specifications, screenshots, wireframes 

├── supabase-local/              # Local Supabase dev environment 

├── waterfront-web/              # Next.js PWA (customer booking app) 

├── waterfront-infra/            # Docker Compose (MQTT broker, future BTCPay) 

├── waterfront-esp32/            # PlatformIO project (ESP32 firmware) 

├── waterfront-backend/          # Optional Node.js backend (if needed) 

└── README.md

## Quick Start – Development Setup

### Prerequisites

- Node.js 20+ (via nvm recommended)
- pnpm (`npm install -g pnpm`)
- Docker Desktop
- Supabase CLI (`brew install supabase/tap/supabase` or `npm install -g supabase`)
- PlatformIO (VS Code extension)
- GitHub account (for repo sync)

### 1. Clone & Install Dependencies

```bash
git clone https://github.com/BBXtreme/Waterfront.git
cd Waterfront

# Install pnpm dependencies for the PWA
cd waterfront-web
pnpm install
```

### 2. Run the PWA (Customer Booking App)

Bash

```
cd waterfront-web
pnpm dev
```

→ Open [http://localhost:3000](http://localhost:3000/) You should see the home screen ("Book Kayak or SUP").

### 3. Start Local Supabase (Database + Auth + Realtime)

Bash

```
cd ~/Documents/GitHub/Waterfront/supabase-local
supabase init    # only first time
supabase start
```

→ Open Studio: [http://127.0.0.1:54323](http://127.0.0.1:54323/) (default credentials: postgres/postgres) → Create tables manually or via migration (see docs/ for schema examples):

- bookings
- machines
- events

Add to waterfront-web/.env.local:

env

```
NEXT_PUBLIC_SUPABASE_URL=http://127.0.0.1:54321 NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJh...
```

Restart pnpm dev → PWA can now connect to local Supabase.

### 4. Start MQTT Broker (Local Testing for ESP32)

Bash

```
cd ~/Documents/GitHub/Waterfront/waterfront-infra
docker compose up -d
```

→ Mosquitto broker running on localhost:1883 (anonymous access for dev) → Test with MQTT Explorer or mosquitto_pub / mosquitto_sub

### 5. ESP32 Development (PlatformIO)

1. Open VS Code → open folder ~/Documents/GitHub/Waterfront/waterfront-esp32
2. PlatformIO extension → Home → New Project (if not already initialized)
   - Board: ESP32 Dev Module (or your board)
   - Framework: Arduino
   - Location: current folder
3. Edit platformio.ini and src/main.cpp (see docs/ for examples)
4. Build / Upload via PlatformIO toolbar (bottom of VS Code)

### Environment Variables (waterfront-web/.env.local)

env

```
# Supabase (local) NEXT_PUBLIC_SUPABASE_URL=http://127.0.0.1:54321 NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJh... # Production Supabase (later) # NEXT_PUBLIC_SUPABASE_URL=https://your-project.supabase.co # NEXT_PUBLIC_SUPABASE_ANON_KEY=eyJh... # MQTT broker (for dev) MQTT_BROKER_URL=mqtt://localhost:1883 # Stripe / BTCPay (add later) STRIPE_SECRET_KEY=sk_test_... BTCPAY_URL=https://your-btcpay-server.com BTCPAY_API_KEY=...
```

### Useful Commands Summary

| Task                    | Command                                     |
| ----------------------- | ------------------------------------------- |
| Run PWA                 | cd waterfront-web && pnpm dev               |
| Start local Supabase    | cd supabase-local && supabase start         |
| Start MQTT broker       | cd waterfront-infra && docker compose up -d |
| Stop MQTT               | cd waterfront-infra && docker compose down  |
| Build ESP32 firmware    | PlatformIO → Build (in VS Code)             |
| Upload to ESP32         | PlatformIO → Upload                         |
| Open project in VS Code | code . (from repo root)                     |

### Development Guidelines

- Use Aider for AI-assisted coding: aider --model xai/grok-code-fast-1
- Commit often with clear messages
- Keep waterfront-web as main focus until PWA is complete
- Document new features in docs/
- Use branches for big features (git checkout -b feat/supabase-integration)

### License

MIT License (to be confirmed)

------

Happy building! Questions → open an Issue in this repo.
