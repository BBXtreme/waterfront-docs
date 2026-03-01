/**
 * @file return_sensor.h
 * @brief Header for ultrasonic return sensor functions.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Provides functions for sensor initialization, distance measurement, and presence detection.
 */

#ifndef RETURN_SENSOR_H
#define RETURN_SENSOR_H

/**
 * @brief Initializes the ultrasonic sensor pins.
 */
void sensor_init();

/**
 * @brief Gets the distance measured by the sensor in cm.
 * @return Distance in cm, or -1.0f on error.
 */
float sensor_get_distance();

/**
 * @brief Checks if a kayak is present based on distance threshold.
 * @return True if kayak is present, false otherwise.
 */
bool sensor_is_kayak_present();

#endif // RETURN_SENSOR_H
