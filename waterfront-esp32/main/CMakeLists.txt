idf_component_register(
    SRCS
        "main.cpp"
        "lte_manager.cpp"
        "mqtt_client.cpp"
        "nimble.cpp"              # ← stubbed below
        "offline_fallback.cpp"
        "ota_handler.cpp"
        "provisioning.cpp"
        "relay_handler.cpp"
        "return_sensor.cpp"
        "wifi_manager.cpp"
        "error_handler.cpp"
        # "power_manager.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        esp_wifi
        nvs_flash
        mqtt
        esp-tls
        esp_http_client
        esp_https_ota
        app_update
        json
        driver
        esp_adc
        esp_event
        esp_netif
        freertos
        lwip
        bt                    # needed for esp_bt.h
        spiffs
        wifi_provisioning
)