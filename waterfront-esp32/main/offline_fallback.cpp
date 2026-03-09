#include "offline_fallback.h"
#include "config_loader.h"   // assuming this exists
#include <string.h>
#include "cJSON.h"
#include "nvs_flash.h"

#define TAG "OFFLINE"
#define NVS_NAMESPACE "pins"
#define MAX_BOOKINGS 10

typedef struct {
    char bookingId[32];
    char pin[8];
    time_t expires;
} BookingEntry;

static BookingEntry activeBookings[MAX_BOOKINGS];
static int numActiveBookings = 0;

void offline_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
    ESP_LOGI(TAG, "NVS initialized for offline PINs");
}

void offline_sync_pins(const char* payload) {
    if (!payload) return;

    cJSON* root = cJSON_Parse(payload);
    if (!root) {
        ESP_LOGE(TAG, "JSON parse error");
        return;
    }

    numActiveBookings = 0;
    cJSON* item;
    cJSON_ArrayForEach(item, root) {
        if (numActiveBookings >= MAX_BOOKINGS) break;

        cJSON* bookingId = cJSON_GetObjectItem(item, "bookingId");
        cJSON* pin       = cJSON_GetObjectItem(item, "pin");
        cJSON* expires   = cJSON_GetObjectItem(item, "expires");

        if (bookingId && pin) {
            strlcpy(activeBookings[numActiveBookings].bookingId, bookingId->valuestring, 32);
            strlcpy(activeBookings[numActiveBookings].pin, pin->valuestring, 8);
            activeBookings[numActiveBookings].expires = time(NULL) + 86400; // placeholder
            numActiveBookings++;
        }
    }

    cJSON_Delete(root);

    // Save to NVS
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_blob(h, "bookings", activeBookings, sizeof(BookingEntry) * numActiveBookings);
        nvs_commit(h);
        nvs_close(h);
        ESP_LOGI(TAG, "Synced %d bookings to NVS", numActiveBookings);
    }
}

bool offline_validate_pin(const char* enteredPin) {
    if (!enteredPin) return false;
    time_t now = time(NULL);
    for (int i = 0; i < numActiveBookings; i++) {
        if (strcmp(activeBookings[i].pin, enteredPin) == 0 && activeBookings[i].expires > now) {
            ESP_LOGI(TAG, "Offline PIN validated");
            return true;
        }
    }
    ESP_LOGW(TAG, "Offline PIN invalid");
    return false;
}

void offline_load_pins(void) {
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) return;

    size_t len = sizeof(BookingEntry) * MAX_BOOKINGS;
    if (nvs_get_blob(h, "bookings", activeBookings, &len) == ESP_OK) {
        numActiveBookings = len / sizeof(BookingEntry);
        ESP_LOGI(TAG, "Loaded %d bookings from NVS", numActiveBookings);
    }
    nvs_close(h);
}

void offline_cleanup_expired(void) {
    time_t now = time(NULL);
    int j = 0;
    for (int i = 0; i < numActiveBookings; i++) {
        if (activeBookings[i].expires > now) {
            activeBookings[j++] = activeBookings[i];
        }
    }
    numActiveBookings = j;
    ESP_LOGI(TAG, "Cleaned expired bookings, %d left", numActiveBookings);
}