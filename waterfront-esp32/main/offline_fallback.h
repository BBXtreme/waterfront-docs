#ifndef OFFLINE_FALLBACK_H
#define OFFLINE_FALLBACK_H

#include <esp_log.h>
#include <nvs.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

void offline_init(void);
void offline_sync_pins(const char* payload);
bool offline_validate_pin(const char* enteredPin);
void offline_load_pins(void);
void offline_cleanup_expired(void);

#ifdef __cplusplus
}
#endif

#endif // OFFLINE_FALLBACK_H