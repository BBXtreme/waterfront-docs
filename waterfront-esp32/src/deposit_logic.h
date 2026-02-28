/**
 * @file deposit_logic.h
 * @brief Header for deposit hold/release logic in WATERFRONT ESP32.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Manages rental timers and overdue auto-lock.
 */

#ifndef DEPOSIT_LOGIC_H
#define DEPOSIT_LOGIC_H

#include <vector>

// Struct for rental timer
struct RentalTimer {
    int compartmentId;
    unsigned long startMs;
    unsigned long durationSec;
};

// Vector of active timers
extern std::vector<RentalTimer> activeTimers;

/**
 * @brief Initializes deposit logic.
 */
void deposit_init();

/**
 * @brief Starts a rental timer for a compartment.
 * @param compartmentId The compartment ID.
 * @param durationSec The rental duration in seconds.
 */
void startRental(int compartmentId, unsigned long durationSec);

/**
 * @brief Checks for overdue rentals and auto-locks.
 */
void checkOverdue();

/**
 * @brief On kayak taken: hold deposit, start timer.
 */
void deposit_on_take();

/**
 * @brief On kayak returned: check timing, release deposit if on time.
 * @param client MQTT client for publishing.
 */
void deposit_on_return(PubSubClient* client);

/**
 * @brief Gets deposit status.
 * @return True if deposit is held.
 */
bool deposit_is_held();

#endif // DEPOSIT_LOGIC_H
