// return_sensor.cpp - Ultrasonic sensor for kayak presence detection
// This file handles HC-SR04 ultrasonic sensor to detect if a kayak is present in the bay.
// It measures distance and determines presence based on threshold.
// Used for real-time slot booking sync and gate control.
// Added calibration for environmental temperature and humidity compensation.
// Adaptive thresholds based on environmental conditions.

#include "return_sensor.h"
#include "config_loader.h"
#include <driver/gpio.h>
#include <esp_timer.h>

// Default environmental conditions (can be updated via config or sensor)
static float ambientTemperatureC = 20.0f;  // Ambient temperature in Celsius
static float ambientHumidityPercent = 50.0f;  // Ambient humidity in percent

// Adaptive presence threshold (adjusted based on conditions)
static float presenceThresholdCm = 50.0f;  // Base threshold in cm

// Mutex for thread-safe access to sensor variables
portMUX_TYPE sensorMutex = portMUX_INITIALIZER_UNLOCKED;

// Update environmental conditions (call periodically or from sensor)
void sensor_update_environment(float tempC, float humidityPercent) {
    vPortEnterCritical(&sensorMutex);
    ambientTemperatureC = tempC;
    ambientHumidityPercent = humidityPercent;
    // Adaptive threshold: increase slightly in high humidity or temp for better accuracy
    presenceThresholdCm = 50.0f + (humidityPercent - 50.0f) * 0.1f + (tempC - 20.0f) * 0.05f;
    if (presenceThresholdCm < 40.0f) presenceThresholdCm = 40.0f;  // Min threshold
    if (presenceThresholdCm > 60.0f) presenceThresholdCm = 60.0f;  // Max threshold
    vPortExitCritical(&sensorMutex);
}

// Calculate speed of sound in m/s based on temperature and humidity
// Formula: speed = 331.3 + 0.606 * T + 0.0124 * H (approximate)
float calculate_speed_of_sound() {
    vPortEnterCritical(&sensorMutex);
    float speed = 331.3f + 0.606f * ambientTemperatureC + 0.0124f * ambientHumidityPercent;
    vPortExitCritical(&sensorMutex);
    return speed;
}

// Initialize sensor pins
void sensor_init() {
    // Use pins from config for the first compartment (assuming single sensor for now)
    if (g_config.compartmentCount > 0) {
        gpio_set_direction((gpio_num_t)g_config.compartments[0].ultrasonicTriggerPin, GPIO_MODE_OUTPUT);
        gpio_set_direction((gpio_num_t)g_config.compartments[0].ultrasonicEchoPin, GPIO_MODE_INPUT);
        ESP_LOGI("SENSOR", "Ultrasonic sensor initialized on TRIG %d, ECHO %d", g_config.compartments[0].ultrasonicTriggerPin, g_config.compartments[0].ultrasonicEchoPin);
    }
}

// Get distance in cm with environmental compensation
float sensor_get_distance() {
    if (g_config.compartmentCount == 0) return -1.0f;
    gpio_num_t trigPin = (gpio_num_t)g_config.compartments[0].ultrasonicTriggerPin;
    gpio_num_t echoPin = (gpio_num_t)g_config.compartments[0].ultrasonicEchoPin;

    // Trigger pulse
    gpio_set_level(trigPin, 0);
    ets_delay_us(2);
    gpio_set_level(trigPin, 1);
    ets_delay_us(10);
    gpio_set_level(trigPin, 0);

    // Measure echo
    uint64_t start = esp_timer_get_time();
    while (gpio_get_level(echoPin) == 0) {
        if (esp_timer_get_time() - start > 1000000) return -1.0f;  // Timeout 1ms
    }
    start = esp_timer_get_time();
    while (gpio_get_level(echoPin) == 1) {
        if (esp_timer_get_time() - start > 1000000) return -1.0f;  // Timeout 1ms
    }
    uint64_t duration = esp_timer_get_time() - start;

    // Calculate distance with compensated speed of sound
    float speedOfSound = calculate_speed_of_sound();  // m/s
    float distance = (duration / 1000000.0f) * speedOfSound / 2.0f * 100.0f;  // Convert to cm
    return distance;
}

// Check if kayak is present using adaptive threshold
bool sensor_is_kayak_present() {
    float dist = sensor_get_distance();
    vPortEnterCritical(&sensorMutex);
    bool present = dist < presenceThresholdCm && dist > 0;  // >0 to filter invalid readings
    vPortExitCritical(&sensorMutex);
    return present;
}
