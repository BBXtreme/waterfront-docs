// lte_manager.h - LTE cellular fallback using TinyGSM

#ifndef LTE_MANAGER_H
#define LTE_MANAGER_H

#include <TinyGsmClient.h>
#include <HardwareSerial.h>

// Externs for global use
extern TinyGsm modem;
extern TinyGsmClient lteClient;
extern HardwareSerial SerialAT;

void initLTE();
void powerUpModem();
void powerDownModem();
void switchToLTE();
void switchToWiFi();

#endif // LTE_MANAGER_H
