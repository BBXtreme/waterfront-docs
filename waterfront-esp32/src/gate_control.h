// gate_control.h - Header for gate control functions
// This header declares functions for servo and limit switch operations.
// Used for physical gate control in slot-based system.

#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

// Initialize gate control for all slots
void gate_init();

// Open gate for slot (non-blocking, sets target state)
void openGateServo(int slotId);

// Close gate for slot (non-blocking, sets target state)
void closeGateServo(int slotId);

// Get gate state
const char* getGateState(int slotId);

// Task to handle servo movement (call from main loop)
void gate_task();

#endif // GATE_CONTROL_H
