# Waterfront  
**Kayak / SUP Rental Booking App + ESP32 MQTT Controller**

Self-service, unmanned rental system with Next.js PWA, Supabase backend, BTCPay/Stripe payments, and ESP32-based smart locker control via MQTT.

Live demo (Vercel/Railway....tbd): https://waterfront-[your-project-slug].vercel.app  

**Current status**: Early development – auth & local setup working, calendar & payments next

## Features (Current & Planned)

- Guest booking (no login required for basic flow)
- Calendar + time slot selection
- Location / equipment selection
- Real-time availability (Supabase Realtime)
- Payments: Stripe (fiat) + BTCPay Server (Lightning / Liquid BTC)
- Instant PIN + QR code confirmation (via email or on-screen)
- ESP32 MQTT integration for locker unlock / sensor feedback (take/return events)
- Admin dashboard (telemetry, bookings, payments, logs)
- Offline-tolerant PWA (cached QR/PIN for poor connectivity)

## Tech Stack

- **Frontend / PWA** — Next.js 16+, TypeScript, Tailwind CSS, shadcn/ui  
- **Backend / Database** — Supabase (PostgreSQL + Auth + Realtime + Storage)  
- **Payments** — Stripe + BTCPay Server (self-hosted)  
- **IoT / Hardware** — ESP32 (Arduino framework + PlatformIO), PubSubClient (MQTT), TinyGSM (LTE fallback)  
- **Infrastructure** — Docker + Mosquitto MQTT broker  
- **Development** — pnpm, Aider (AI coding), GitHub  
- **Deployment** — Vercel (frontend only)

## Project Structure (Monorepo)

Waterfront/ 

├── docs/                 # Specifications, screenshots, wireframes, FSD/TSD 

├── supabase-local/       # Local Supabase dev environment (CLI) 

├── waterfront-web/       # Next.js PWA → deployed to Vercel 

├── waterfront-infra/     # Docker Compose (MQTT broker, future BTCPay, etc.) 

├── waterfront-esp32/     # PlatformIO project (ESP32 firmware) 

├── waterfront-backend/   # Optional Node.js/Edge Functions (if needed) 

└── README.md

**Note**: Only `waterfront-web` is deployed to Vercel. The rest (ESP32, infra, local Supabase) remain local/dev tools.

## Quick Start – Development Setup

### Prerequisites

- Node.js 20+ (via nvm recommended)
- pnpm (`npm install -g pnpm`)
- Docker Desktop
- Supabase CLI (`brew install supabase/tap/supabase` or `npm install -g supabase`)
- PlatformIO (VS Code extension)
- GitHub account

### 1. Clone & Install

```bash
git clone https://github.com/BBXtreme/Waterfront.git
cd Waterfront

cd waterfront-web
pnpm install
```

### 2. Run the PWA locally

Bash

```
cd waterfront-web
pnpm dev
```

→ [http://localhost:3000](http://localhost:3000/)

### 3. Local Supabase (Database + Auth + Realtime)

Bash

```
cd ../supabase-local
supabase init    # only first time
supabase start
```

→ Studio: [http://127.0.0.1:54323](http://127.0.0.1:54323/) Default login: postgres / postgres → Create tables / run migrations (see docs/ for schema)

### 4. Local MQTT Broker (for ESP32 testing)

Bash

```
cd ../waterfront-infra
docker compose up -d
```

→ Mosquitto at mqtt://localhost:1883 (anonymous)

### 5. ESP32 Firmware (PlatformIO)

Open waterfront-esp32 folder in VS Code → use PlatformIO toolbar to build/upload.

## Deployment – (waterfront-web only)

tbd

## Environment Variables

### Local – waterfront-web/.env.local

Add to waterfront-web/.env.local:

```
# Local Supabase – API endpoint MUST be 54321 (Project URL)
NEXT_PUBLIC_SUPABASE_URL=http://127.0.0.1:54321

# Use the Publishable key (this is your public/anon key for client-side)
NEXT_PUBLIC_SUPABASE_ANON_KEY=sb_publishable_ACJWlz.....

# Optional: only if you do server-side admin operations (e.g. bypass RLS)
SUPABASE_SERVICE_ROLE_KEY=sb_secret_N7U........

# MQTT (local dev) 
MQTT_BROKER_URL=mqtt://localhost:1883 

# Payments (add when ready) 
NEXT_PUBLIC_STRIPE_PUBLISHABLE_KEY=pk_test_... 
STRIPE_SECRET_KEY=sk_test_... 
BTCPAY_URL=https://your-btcpay-server.com BTCPAY_API_KEY=...
```
**Rule of thumb**: NEXT_PUBLIC_ → client-side (browser), others → server/edge only.

## Useful Commands

| Task                 | Command                                     |
| -------------------- | ------------------------------------------- |
| Run PWA              | cd waterfront-web && pnpm dev               |
| Start local Supabase | cd supabase-local && supabase start         |
| Start MQTT broker    | cd waterfront-infra && docker compose up -d |
| Stop MQTT            | cd waterfront-infra && docker compose down  |
| Open whole project   | code . (from root)                          |

## Development Guidelines

- Use Aider: aider --model xai/grok-code-fast-1
- Commit often, small & clear messages
- Keep waterfront-web as priority until PWA flow is complete
- Document decisions & schemas in docs/
- Feature branches: git checkout -b feat/calendar-availability

## License

MIT License (to be confirmed)

Happy building! Questions → open an Issue
