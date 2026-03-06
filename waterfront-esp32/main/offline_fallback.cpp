// offline_fallback.cpp - Offline PIN validation and fallback logic
// This file handles pre-synced PIN lists from retained MQTT topics for offline unlock validation.
// It stores active bookings/PINs in NVS and validates entered PINs locally if MQTT is unavailable.
// Used for gate control when the ESP32 cannot reach the broker.

#include "offline_fallback.h"
#include "config_loader.h"
#include <nvs.h>
#include <ArduinoJson.h>

// NVS namespace for PIN storage
#define NVS_NAMESPACE "pins"

// Structure for a single booking entry
struct BookingEntry {
    char bookingId[32];
    char pin[8];
    time_t expires;  // Unix timestamp for expiration
};

// Array to hold active bookings (max 10 for simplicity; adjust as needed)
#define MAX_BOOKINGS 10
BookingEntry activeBookings[MAX_BOOKINGS];
int numActiveBookings = 0;

// Initialize NVS for PIN storage
void offline_init() {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI("OFFLINE", "NVS initialized for PIN storage");
}

// Sync PIN list from retained MQTT topic payload
// Payload example: [{"bookingId":"bk_abc","pin":"1234","expires":"2026-03-01T12:00:00Z"}, ...]
void offline_sync_pins(const char* payload) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        ESP_LOGE("OFFLINE", "Failed to parse PIN sync payload");
        return;
    }

    numActiveBookings = 0;
    for (JsonObject booking : doc.as<JsonArray>()) {
        if (numActiveBookings >= MAX_BOOKINGS) break;
        strlcpy(activeBookings[numActiveBookings].bookingId, booking["bookingId"], sizeof(activeBookings[numActiveBookings].bookingId));
        strlcpy(activeBookings[numActiveBookings].pin, booking["pin"], sizeof(activeBookings[numActiveBookings].pin));
        // Parse expires timestamp (assume ISO 8601; simple conversion)
        String expiresStr = booking["expires"];
        // TODO: Implement proper ISO 8601 to Unix timestamp conversion
        activeBookings[numActiveBookings].expires = 0;  // Placeholder; set to future time
        numActiveBookings++;
    }

    // Store in NVS for persistence
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE("OFFLINE", "NVS open failed");
        return;
    }
    size_t size = sizeof(BookingEntry) * numActiveBookings;
    err = nvs_set_blob(nvs_handle, "bookings", activeBookings, size);
    if (err != ESP_OK) {
        ESP_LOGE("OFFLINE", "NVS set blob failed");
    }
    nvs_commit(nvs_handle);
    nvs_close(nvs_handle);
    ESP_LOGI("OFFLINE", "Synced %d bookings to NVS", numActiveBookings);
}

// Validate PIN locally (for offline keypad entry or MQTT fallback)
bool offline_validate_pin(const char* enteredPin) {
    time_t now = time(NULL);  // Assume time is set; otherwise use millis-based check
    for (int i = 0; i < numActiveBookings; i++) {
        if (strcmp(activeBookings[i].pin, enteredPin) == 0 && activeBookings[i].expires > now) {
            ESP_LOGI("OFFLINE", "PIN validated offline for booking %s", activeBookings[i].bookingId);
            return true;
        }
    }
    ESP_LOGW("OFFLINE", "PIN validation failed offline");
    return false;
}

// Load PINs from NVS on boot
void offline_load_pins() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW("OFFLINE", "NVS open for load failed");
        return;
    }
    size_t size = sizeof(BookingEntry) * MAX_BOOKINGS;
    err = nvs_get_blob(nvs_handle, "bookings", activeBookings, &size);
    if (err == ESP_OK) {
        numActiveBookings = size / sizeof(BookingEntry);
        ESP_LOGI("OFFLINE", "Loaded %d bookings from NVS", numActiveBookings);
    } else {
        ESP_LOGW("OFFLINE", "NVS get blob failed");
    }
    nvs_close(nvs_handle);
}

// Clean up expired bookings (call periodically)
void offline_cleanup_expired() {
    time_t now = time(NULL);
    int newCount = 0;
    for (int i = 0; i < numActiveBookings; i++) {
        if (activeBookings[i].expires > now) {
            activeBookings[newCount++] = activeBookings[i];
        }
    }
    numActiveBookings = newCount;
    ESP_LOGI("OFFLINE", "Cleaned up expired bookings; %d remaining", numActiveBookings);
}
