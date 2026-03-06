// gate_control.h - Header for gate control functions
// This header declares functions for servo and limit switch operations.
// Used for physical gate control in slot-based system.
// Author: BBXtreme + Grok
// Date: 2026-02-28
// Note: Provides non-blocking gate operations for kayak compartments.

#ifndef GATE_CONTROL_H
#define GATE_CONTROL_H

#include "config.h"

// State machine for each compartment
enum CompartmentState { IDLE, OPENING, CLOSING, OPEN, CLOSED, ERROR }; ///< States for compartment gate control

// Extern compartment states for testing
extern CompartmentState compartmentStates[MAX_COMPARTMENTS]; ///< Array of compartment states
extern unsigned long compartmentStartTimes[MAX_COMPARTMENTS]; ///< Start times for operations
extern int retryCounts[MAX_COMPARTMENTS]; ///< Retry counts for failed operations

/**
 * @brief Initializes gate control for all compartments.
 *        Sets up servos, limit switches, and initial states based on configuration.
 */
void gate_init();

/**
 * @brief Opens the gate for a specific compartment (non-blocking operation).
 *        Sets the target state to open; actual movement is handled in gate_task().
 * @param compartmentId The ID of the compartment to open (1-based index).
 */
void openCompartmentGate(int compartmentId);

/**
 * @brief Closes the gate for a specific compartment (non-blocking operation).
 *        Sets the target state to close; actual movement is handled in gate_task().
 * @param compartmentId The ID of the compartment to close (1-based index).
 */
void closeCompartmentGate(int compartmentId);

/**
 * @brief Retrieves the current gate state for a specific compartment.
 * @param compartmentId The ID of the compartment to query (1-based index).
 * @return A string representing the gate state ("open", "closed", or "unknown").
 */
const char* getCompartmentGateState(int compartmentId);

/**
 * @brief Task function to handle servo movement and state updates.
 *        Should be called periodically from the main loop to process gate operations.
 *        Checks limit switches and updates states accordingly.
 */
void gate_task();

#endif // GATE_CONTROL_H
