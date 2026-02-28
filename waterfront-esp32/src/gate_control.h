// gate_control.h - Header for gate control functions
// This header declares functions for servo and limit switch operations.
// Used for physical gate control in slot-based system.

#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

// Initialize gate control
void gate_init();

// Open gate for slot
void openGateServo(int slotId);

// Close gate for slot
void closeGateServo(int slotId);

// Get gate state
const char* getGateState(int slotId);

#endif // GATE_CONTROL_H
