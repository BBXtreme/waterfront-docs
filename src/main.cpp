#include <Arduino.h>
#include "mqtt_base.cpp"  // Integrate parts

void setup() {
    // Call mqtt_base setup
    ::setup();
}

void loop() {
    // Call mqtt_base loop
    ::loop();
}
