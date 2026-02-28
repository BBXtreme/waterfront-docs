#include <Arduino.h>

void setup() {
    Serial.begin(115200);
    Serial.println("WATERFRONT-ESP32: Hello from Waterfront ESP32 Kayak Controller!");
    printf("Build successful - base ESP-IDF setup ready for MQTT integration.\n");
}

void loop() {
    Serial.println("WATERFRONT-ESP32: Still alive...");
    delay(5000);  // ~5 seconds delay
}
