#ifndef CONFIG_H
#define CONFIG_H

// WiFi and MQTT settings
#define WIFI_SSID "default_ssid"
#define WIFI_PASS "default_pass"
#define MQTT_SERVER "broker.emqx.io"
#define MQTT_PORT 1883
#define MQTT_SUBSCRIBE_TOPIC "/kayak/test/unlock"
#define MQTT_PUBLISH_TOPIC "/kayak/test/status"

// BLE settings (from nimble.h and bleprph.h)
#define BLE_DEVICE_NAME "KayakDevice"
#define MAX_DEVICES 100

// GPIO pins (example, adjust as needed)
#define PIN_LED 2

#endif // CONFIG_H
