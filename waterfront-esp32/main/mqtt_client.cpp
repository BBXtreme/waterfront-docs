#include "mqtt_client.h"
#include "config_loader.h"
#include <esp_log.h>
#include <mqtt_client.h>
#include "cJSON.h"

#define TAG "MQTT"

esp_mqtt_client_handle_t mqttClient = NULL;
bool mqttConnected = false;

static void event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_mqtt_event_handle_t event = event_data;
    if (event_id == MQTT_EVENT_CONNECTED) {
        mqttConnected = true;
        ESP_LOGI(TAG, "MQTT Connected");
        // Subscribe to topics here later
    } else if (event_id == MQTT_EVENT_DISCONNECTED) {
        mqttConnected = false;
        ESP_LOGW(TAG, "MQTT Disconnected");
    }
}

esp_err_t mqtt_init() {
    esp_mqtt_client_config_t cfg = {
        .broker = {
            .address.uri = "mqtts://8bee884b3e6048c280526f54fe81b9b9.s1.eu.hivemq.cloud",
            .address.port = 8883,
            .credentials.username = "mqttuser",
            .credentials.password = "strongpass123",
        },
        .session = {
            .disable_clean_session = true,
        },
    };

    mqttClient = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqttClient, ESP_EVENT_ANY_ID, event_handler, NULL);
    return esp_mqtt_client_start(mqttClient);
}

void mqtt_publish_status() {
    cJSON* root = cJSON_CreateObject();
    if (!root) return;

    cJSON_AddStringToObject(root, "status", "online");
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000000);

    char* str = cJSON_PrintUnformatted(root);
    esp_mqtt_client_publish(mqttClient, "waterfront/status", str, 0, 1, 0);
    cJSON_free(str);
    cJSON_Delete(root);
}

// Stub other functions
void mqtt_publish_slot_status(int slotId, const char* jsonPayload) {}
void mqtt_publish_retained_status(int compartmentId, const char* jsonPayload) {}
void mqtt_publish_ack(int compartmentId, const char* action) {}
bool mqtt_connect() { return mqttConnected; }
void mqtt_subscribe() {}
void mqtt_loop() {}
void mqtt_loop_task(void *pvParameters) { vTaskDelete(NULL); }
int getMqttReconnectCount() { return 0; }