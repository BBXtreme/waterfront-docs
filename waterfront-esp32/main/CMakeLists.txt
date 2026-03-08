# main/CMakeLists.txt
idf_component_register(
    SRCS
        "main.cpp"
        "lte_manager.cpp"
        "MQTT_Client.cpp"
        "nimble.cpp"
        "offline_fallback.cpp"
        "ota_handler.cpp"
        "provisioning.cpp"
        "relay_handler.cpp"
        "return_sensor.cpp"
        "wifi_manager.cpp"
        "error_handler.cpp"
        # "power_manager.cpp"     # still commented

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        nvs_flash
        mqtt
        esp-tls
        esp_http_client
        esp_https_ota
        app_update              # correct for esp_ota_ops.h
        json
        driver
        esp_adc
        esp_event
        esp_netif
        freertos
        lwip
        bt
        spiffs
        wifi_provisioning
)