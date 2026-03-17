#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <stdbool.h>          // bool in C
#include <esp_event.h>
#include <esp_mqtt_client.h>

#ifdef __cplusplus
extern "C" {
#endif

extern esp_mqtt_client_handle_t mqttClient;
extern bool mqttConnected;

esp_err_t mqtt_init(void);
void event_handler(void *handler_args,
                   esp_event_base_t base,
                   int32_t event_id,
                   void *event_data);
void mqtt_publish_status(void);
void mqtt_publish_slot_status(int slotId, const char *jsonPayload);
void mqtt_publish_retained_status(int compartmentId,
                                  const char *jsonPayload);
void mqtt_publish_ack(int compartmentId, const char *action);
bool mqtt_connect(void);
void mqtt_subscribe(void);
void mqtt_loop(void);
void mqtt_loop_task(void *pvParameters);
int getMqttReconnectCount(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_H