#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>

void wifi_init();
void wifi_event_handler(WiFiEvent_t event);

#endif // WIFI_MANAGER_H
