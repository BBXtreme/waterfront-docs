# main/CMakeLists.txt
idf_component_register(
    SRCS
        "main.cpp"
        "lte_manager.cpp"
        "MQTT_Client.cpp"           # fixed case
        "nimble.cpp"
        "offline_fallback.cpp"
        "ota_handler.cpp"
        "power_manager.cpp"
        "provisioning.cpp"
        "relay_handler.cpp"
        "return_sensor.cpp"
        "wifi_manager.cpp"
        "error_handler.cpp"         # added if you have it

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        nvs_flash
        mqtt
        esp-tls
        esp_http_client
        esp_https_ota
        json                # cJSON
        driver
        esp_adc
        esp_event
        esp_netif
        freertos
        lwip
        bt
        spiffs              # for esp_spiffs.h
        wifi_provisioning   # for BLE provisioning
)