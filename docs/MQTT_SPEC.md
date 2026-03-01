# MQTT Specification – WATERFRONT Controller

**Purpose**: exact topics and JSON schemas. This is critical because backend (Supabase/webhook) and ESP32 must speak the same language.

Broker: [your Mosquitto / EMQX / HiveMQ address:port]  
TLS: planned for production  
QoS: 1 or 2 for commands, 0 for telemetry

## Topics

Inbound (backend → ESP32)
- `waterfront/locations/{locationCode}/compartments/{compartmentNumber}/status`          QoS 1   JSON payload (retained)
- `waterfront/locations/{locationCode}/compartments/{compartmentNumber}/command`         QoS 1   JSON payload

Outbound (ESP32 → backend)
- `waterfront/locations/{locationCode}/status`     QoS 0   location telemetry (retained) – provisioning, IP, conn type, battery etc.
- `waterfront/locations/{locationCode}/compartments/{compartmentNumber}/status`          QoS 0   compartment telemetry (retained) – booked, gateState, bookingId etc.
- `waterfront/locations/{locationCode}/compartments/{compartmentNumber}/event`           QoS 1   taken / returned / error
- `waterfront/locations/{locationCode}/compartments/{compartmentNumber}/ack`             QoS 1   acknowledgments

## Payload Schemas (JSON)

Location status (retained) example:
```json
{
  "state": "idle",
  "battery": 92,
  "connType": "wifi"
}
```

Compartment status (retained) example:
```json
{
  "compartmentNumber": 1,
  "booked": true,
  "bookingId": "BKG-20260228-001",
  "startTime": "2026-03-01T10:00:00Z",
  "endTime": "2026-03-01T12:00:00Z",
  "customerName": "Marco Rossi",
  "gateState": "locked"
}
```

Command example:
```json
"open_gate"
```

Event (on take/return/error):

JSON

```
{
  "compartmentNumber": 1,
  "event": "taken" | "returned" | "error",
  "timestamp": "2026-02-28T11:43:00Z",
  "details": { "distanceCm": 12 }   // optional sensor data
}
```

Ack example:
```json
{
  "compartmentNumber": 1,
  "action": "gate_opened",
  "timestamp": 1234567890
}
```

## Config Update Topic

Topic: waterfront/{location}/{locationCode}/config/update  
Direction: Backend/Admin → ESP32  
QoS: 1  
Retained: false  
Payload: full JSON matching /config.json schema (example provided)  
Action: ESP32 validates → saves to LittleFS → reloads config globals (optional restart)

Example payload:
```json
{
  "mqtt": {"broker": "192.168.178.50", "port": 1883},
  "location": {"slug": "bremen", "code": "harbor-01"},
  "compartments": [
    {"number":1, "servoPin":12, "limitOpenPin":13, "limitClosePin":14},
    ...
  ],
  "debugMode": true
}
```

Note: This topic allows runtime configuration changes (MQTT broker, compartment pins, debug mode, etc.) without firmware re-flash. Used by admin dashboard.

## Security & Authentication

- **Broker Configuration**: The Mosquitto broker requires username/password authentication (allow_anonymous false) and enforces TLS encryption on port 8883 for secure connections.
- **ESP32 Client Setup**: The ESP32 uses `mqttClient.connect(clientId, username, password)` for authentication. If `useTLS=true` in config.json, it enables TLS with `setSecure(true)` for MQTTS connections over WiFi or LTE.

## Security Notes

- **Authentication**: Username/password required (generate with `mosquitto_passwd -c /mosquitto/config/passwd mqttuser`). ✅ DONE
- **TLS**: Enabled on port 8883 with CA/cert/key files. For self-signed certs (dev): Use openssl commands in mosquitto.conf comments. Production: Use Let's Encrypt. ✅ DONE
- **Client Certs**: Optional (require_certificate false). ✅ DONE
- **WebSocket**: Also secured with auth on port 9001. ✅ DONE
