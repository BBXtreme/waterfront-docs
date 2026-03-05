#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

void wifi_init();
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif // WIFI_MANAGER_H
