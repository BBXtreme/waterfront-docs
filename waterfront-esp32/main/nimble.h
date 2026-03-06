// nimble.h - Header file for BLE (NimBLE) functions
// This header declares functions for initializing and managing BLE provisioning.
// It provides an interface for starting BLE-based WiFi configuration.
// Used in conjunction with nimble.cpp for BLE operations.

#ifndef NIMBLE_H
#define NIMBLE_H

// Initialize BLE with a device name for advertising
// deviceName: The name to advertise (e.g., "WATERFRONT-PROV")
void ble_init(const char* deviceName);

// Set the BLE device name (if server is active)
// deviceName: New name to set
void ble_set_device_name(const char* deviceName);

#endif // NIMBLE_H
