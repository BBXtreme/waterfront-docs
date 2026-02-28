// provisioning.cpp - Provisioning logic

#include "provisioning.h"
#include "nimble.h"
#include "webui_server.h"

void startBLEProvisioning() {
    ble_init("WATERFRONT-PROV");
    provisioningActive = true;
}

void startSoftAPProvisioning() {
    start_softap();
    start_rest_server();
    provisioningActive = true;
}
