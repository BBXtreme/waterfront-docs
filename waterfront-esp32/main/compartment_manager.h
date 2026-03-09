// compartment_manager.h - stub
#ifndef COMPARTMENT_MANAGER_H
#define COMPARTMENT_MANAGER_H

/*
 * Attempt to include the ESP logging header.  The editor/analysis
 * engine previously reported an "include errors detected" message
 * because it couldn't resolve esp_log.h.  Guard against that by
 * falling back to a no-op definition of the logging macro if the
 * header isn't available on the current include path.
 */
#if __has_include(<esp_log.h>)
#include <esp_log.h>
#elif __has_include("esp_log.h")
#include "esp_log.h"
#else
/* simple stub so code still compiles when the real header isn't found */
#define ESP_LOGI(tag, fmt, ...) ((void)0)
#endif

/*
 * keep the implementation inline to avoid multiple-definition issues
 * when this header is pulled in from several translation units.
 */
static inline void compartment_init(void) {
    ESP_LOGI("COMP", "Compartment manager initialized (stub)");
}

#endif // COMPARTMENT_MANAGER_H