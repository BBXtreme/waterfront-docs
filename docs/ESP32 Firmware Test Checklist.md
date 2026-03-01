# WATERFRONT ESP32 Firmware – Test Checklist

### Test List / Acceptance Criteria (30–60 min)

**Purpose**: simple pass/fail criteria for the first milestones. 



###### Phase 1 – Basic Connectivity

- [x] Board boots and prints "WATERFRONT starting..." via Serial
- [x] Connects to WiFi (provisioned or hardcoded)
- [x] Connects to MQTT broker and subscribes to /kayak/{machineId}/unlock
- [x] Publishes status message every 60 s

###### Phase 2 – Lock & Sensor

- [x] Relay pulses HIGH for 1.5 s on valid unlock command
- [x] Sensor detects "taken" → publishes event → auto-locks after timeout
- [x] Sensor detects "returned" → publishes event

###### Phase 3 – Failover & Offline

- [x] Disconnect WiFi → switches to LTE within 30 s
- [x] Offline mode: accepts pre-synced PIN via button press (future)





