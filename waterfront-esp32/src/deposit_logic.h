#ifndef DEPOSIT_LOGIC_H
#define DEPOSIT_LOGIC_H

#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

struct RentalTimer {
    int compartmentId;
    unsigned long startMs;
    unsigned long durationSec;
    TimerHandle_t timerHandle;
};

extern std::vector<RentalTimer> activeTimers;

void deposit_init();
void startRental(int compartmentId, unsigned long durationSec);
void checkOverdue();
void deposit_on_take(PubSubClient* client);
void deposit_on_return(PubSubClient* client);
bool deposit_is_held();

#endif // DEPOSIT_LOGIC_H
