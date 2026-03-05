/**
 * @file config.h
 * @brief Central configuration header for WATERFRONT ESP32 Kayak Rental Controller.
 * @author BBXtreme + Grok
 * @date 2026-02-28
 * @note Contains all global constants, pin definitions, and settings for easy maintenance and adaptation.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Configuration header file for WATERFRONT ESP32 Kayak Rental Controller
// This file contains all global constants, pin definitions, and settings used across the project.
// It centralizes configuration to make the code easier to maintain and adapt for different hardware setups.
// All values are defined as macros for compile-time constants, avoiding runtime overhead.

// Debug settings
#define DEBUG_MODE 1                       ///< Enable debug mode (1 = enabled, 0 = disabled)
#define DEBUG_PUBLISH_INTERVAL_MS 5000     ///< Interval to publish debug traces in ms

// Compartment settings
#define MAX_COMPARTMENTS 10                ///< Maximum number of compartments supported

// Logging settings
#define LOG_LEVEL_DEFAULT 3                ///< Default log level (3 = INFO)

#endif // CONFIG_H
