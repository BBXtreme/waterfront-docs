// gate_control.h - Header for gate control functions
// This header declares functions for servo and limit switch operations.
// Used for physical gate control in slot-based system.

#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

/**
 * @brief Initializes gate control for all compartments.
 */
void gate_init();

/**
 * @brief Opens gate for compartment (non-blocking, sets target state).
 * @param compartmentId The compartment ID to open.
 */
void openCompartmentGate(int compartmentId);

/**
 * @brief Closes gate for compartment (non-blocking, sets target state).
 * @param compartmentId The compartment ID to close.
 */
void closeCompartmentGate(int compartmentId);

/**
 * @brief Gets gate state for compartment.
 * @param compartmentId The compartment ID.
 * @return String representing gate state ("open", "closed", or "unknown").
 */
const char* getCompartmentGateState(int compartmentId);

/**
 * @brief Task to handle servo movement (call from main loop).
 */
void gate_task();

#endif // GATE_CONTROL_H
