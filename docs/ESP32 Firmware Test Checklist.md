### 3. Prepare a Minimal Test List / Acceptance Criteria (30–60 min)

**Purpose**: simple pass/fail criteria for the first milestones. 



Example:

#### WATERFRONT ESP32 Firmware – Test Checklist

###### Phase 1 – Basic Connectivity

- [ ] Board boots and prints "WATERFRONT starting..." via Serial
- [ ] Connects to WiFi (provisioned or hardcoded)
- [ ] Connects to MQTT broker and subscribes to /kayak/{machineId}/unlock
- [ ] Publishes status message every 60 s

###### Phase 2 – Lock & Sensor

- [ ] Relay pulses HIGH for 1.5 s on valid unlock command
- [ ] Sensor detects "taken" → publishes event → auto-locks after timeout
- [ ] Sensor detects "returned" → publishes event

###### Phase 3 – Failover & Offline

- [ ] Disconnect WiFi → switches to LTE within 30 s
- [ ] Offline mode: accepts pre-synced PIN via button press (future)





