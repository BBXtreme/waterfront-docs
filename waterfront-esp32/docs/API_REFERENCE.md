# WATERFRONT ESP32 API Reference

## MQTT API

### Topics

#### Incoming Commands
- `waterfront/{location}/{code}/command`: JSON commands for remote control
  - Example: `{"action": "open_gate", "compartment": 1}`
  - Example: `{"action": "update_config", "config": {...}}`

#### Outgoing Responses
- `waterfront/{location}/{code}/response`: Acknowledgments and status
  - Example: `{"ack": true, "action": "open_gate", "compartment": 1, "status": "success"}`

#### Telemetry
- `waterfront/{location}/{code}/debug`: Health data every 60s
- `waterfront/{location}/{code}/alert`: Fatal errors
- `waterfront/machine/{code}/status`: Machine state
- `waterfront/slots/{id}/status`: Compartment status
- `waterfront/locations/{code}/depositRelease`: Deposit release confirmations
- `waterfront/locations/{code}/returnConfirm`: Return confirmations

### Message Formats

#### Command: Open Gate
