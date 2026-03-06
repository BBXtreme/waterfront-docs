// provisioning.h - Header file for provisioning functions
// This header declares functions for starting WiFi provisioning via BLE or SoftAP.
// It provides a unified interface for provisioning modes.
// Used in conjunction with provisioning.cpp for coordination.

#ifndef PROVISIONING_H
#define PROVISIONING_H

// External flag to track provisioning state
extern bool provisioningActive;

// Start BLE-based provisioning (preferred method)
void startBLEProvisioning();

// Start SoftAP-based provisioning (fallback method)
void startSoftAPProvisioning();

#endif
