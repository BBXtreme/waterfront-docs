In the **Waterfront Kayak Rental Application and Control System**, REST and MQTT serve **distinct but complementary roles** in the overall architecture. They are not alternatives to each other — they are used for different parts of the communication flow. Here's a clear breakdown tailored to this project.

### Key Differences: REST vs MQTT in This Project

| Aspect                    | REST (HTTP/HTTPS)                                            | MQTT                                                         | Why It Matters for Waterfront                                |
| ------------------------- | ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |
| **Communication Model**   | Request-Response (client initiates, server answers)          | Publish-Subscribe (pub/sub) — devices subscribe to topics, backend publishes events | REST is used for user-initiated actions (bookings, admin queries); MQTT is ideal for real-time, asynchronous device control (unlock, status updates). |
| **Connection Type**       | Short-lived (each request opens a new TCP connection or reuses one briefly) | Persistent, long-lived TCP connection (ESP32 stays connected to broker most of the time) | MQTT enables near-instant unlock commands even if the ESP32 is in deep sleep or on poor networks; REST would require polling or long-polling → higher latency/power use. |
| **Directionality**        | Mostly unidirectional per request (client → server, or server → client via webhook) | Bidirectional and multi-party (backend can push to many ESP32s; ESP32s push telemetry back) | Backend can instantly command any machine to unlock after payment; ESP32 reports sensor events (kayak taken/returned) without being polled. |
| **Latency / Real-time**   | Higher (100–500 ms typical round-trip; polling adds delay)   | Very low (often <100 ms end-to-end with QoS 1)               | Critical for unlock experience: user pays → backend publishes MQTT → lock opens in seconds (HEIUKI-style flow). REST polling would feel slow. |
| **Bandwidth / Power**     | Higher overhead (HTTP headers on every request)              | Very lightweight (~2–20 bytes per message + headers)         | ESP32 on solar/battery benefits hugely from MQTT — deep sleep + wake only on MQTT message or timer → longer runtime. |
| **Use Case in Project**   | User/app ↔ Backend (bookings, payments, admin dashboard, fetching status history) | Backend ↔ ESP32 machines (real-time control & telemetry)     | REST handles the PWA frontend + Supabase; MQTT handles the vending machine brains. |
| **Scalability**           | Good for many clients, but polling many devices kills server | Excellent for many devices (broker handles fan-out)          | 10+ machines can all stay connected with low server load via one MQTT broker (Mosquitto/EMQX). |
| **Offline / Reliability** | Client must retry failed requests                            | Broker queues messages (QoS 1 or 2); ESP32 can reconnect and get missed commands | MQTT + retained messages or queued commands handle temporary offline better for unlock / late-return charges. |

**Summary in project terms**  
- **REST** = the interface for humans and the booking flow (PWA → Supabase/Next.js backend).  
- **MQTT** = the nervous system for the physical machines (backend → ESP32 → locks/sensors → backend).  

They work together:  
1. User books & pays via PWA → REST API call to backend.  
2. Backend confirms payment (Stripe/BTCPay webhook) → publishes MQTT message to `/waterfront/{machineId}/unlock`.  
3. ESP32 (subscribed to that topic) receives message → opens lock → publishes status/event back via MQTT.  
4. Admin dashboard reads real-time telemetry from MQTT or queries historical data via REST.

### What Is Needed to Run REST in This Project?

REST is already the primary protocol for the **backend ↔ frontend / admin / payment services** part. Here's exactly what's required:

1. **HTTP/HTTPS Server**  
   - Node.js + Express.js (or Next.js API routes) running on your backend (e.g., Render, Vercel, Railway, or a VPS).  
   - Example endpoints you already plan or adapt from nodestark references:  
     - `POST /api/bookings` → create booking, initiate payment  
     - `GET /api/bookings/:id` → get booking details / PIN / QR  
     - `GET /api/machines` → list machines + current status (fetched from MQTT or DB cache)  
     - `GET /api/revenue` or `/api/bookings/stats` → adapted from `/sales` for reports  

2. **Connection / Protocol Requirements**  
   - **TCP port 443 (HTTPS)** — almost always needed in production (free Let's Encrypt cert via certbot, or platform-provided TLS).  
   - **No persistent connection** needed from ESP32 side — REST is only between PWA/users/admins and the backend server.  
   - **CORS** headers if PWA is on a different domain (e.g., `app.waterfront-bremen.de` calling backend on `api.waterfront-bremen.de`).  
   - **Authentication**: JWT (Supabase Auth) or API keys for protected endpoints.  

3. **Webhooks (Special REST case)**  
   - Stripe and BTCPay send **POST** requests to your backend (e.g., `https://your-backend.com/webhooks/stripe` or `/webhooks/btcpay`).  
   - Backend verifies signature → processes payment success → publishes MQTT unlock command.  
   - Requires a publicly reachable HTTPS endpoint (ngrok for local dev, proper domain + TLS in production).

4. **No Extra Protocol for ESP32**  
   - The ESP32 **does not need to speak REST/HTTP** at all in this design (and shouldn't — it would waste power and add complexity).  
   - Only the backend server exposes REST endpoints for the PWA and external services (Stripe, BTCPay, admin tools).

### Quick Architecture Recap (Waterfront Context)

```
PWA (Next.js) ── REST/HTTPS ── Backend (Node.js/Supabase) ── MQTT (persistent) ── ESP32 machines
                               │                                 │
                               └─ Webhooks (Stripe/BTCPay)        └─ Sensors / Relay control
```

- Use **REST** for everything user-facing and state-changing from the app/admin.  
- Use **MQTT** for low-latency, low-power, real-time machine events and commands.

This separation keeps the system clean, scalable, and power-efficient — exactly what the TSD and FSD aim for in a solar-powered, unmanned kayak vending setup in Bremen. If you'd like code examples (Express endpoint + MQTT publish on webhook, or ESP32 MQTT subscribe logic), just let me know which part to focus on next.
```

docs/TSD.md
