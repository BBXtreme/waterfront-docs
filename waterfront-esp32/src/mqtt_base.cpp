// Adapted from app_main in mdb-slave-esp32s3.c, integrated for MQTT base
#include "config.h"
#include "wifi_manager.h"
#include "mqtt_client.h"
#include "nimble.h"
#include "webui_server.h"

void setup() {
    Serial.begin(115200);
    wifi_init();
    ble_init(BLE_DEVICE_NAME);
    start_softap();
    start_rest_server();
    mqtt_init();
}

void loop() {
    mqtt_loop();
    static unsigned long lastPublish = 0;
    if (esp_timer_get_time() / 1000 - lastPublish > 60000) {
        mqtt_publish_status();
        lastPublish = esp_timer_get_time() / 1000;
    }
}
