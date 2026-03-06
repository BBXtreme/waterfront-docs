# waterfront-esp32/main/CMakeLists.txt
# This registers your application code as the "main" component

idf_component_register(
    SRCS
        "main.cpp"
        "lte_manager.cpp"
        "mqtt_client.cpp"           # adjust filename if it's mqtt_client_dir.cpp or similar
        "nimble.cpp"
        "offline_fallback.cpp"
        "ota_handler.cpp"
        "power_manager.cpp"
        "provisioning.cpp"
        "relay_handler.cpp"
        "return_sensor.cpp"
        "wifi_manager.cpp"
        # ↑ add every .cpp file you have in this folder
        # ↓ do NOT list .h files here

    INCLUDE_DIRS
        "."                         # current folder = where your .h files are

    REQUIRES
        esp_wifi                    # WiFi + SoftAP/BLE provisioning
        nvs_flash                   # store WiFi credentials, config
        mqtt                        # MQTT client
        esp-tls                     # secure MQTT (TLS)
        esp_http_client             # for OTA
        json                        # parsing config.json or payloads
        driver                      # GPIO, UART (for LTE), ADC (battery)
        esp_event
        esp_netif
        freertos
        lwip
        bt                          # BLE (provisioning)
        # Optional / add later:
        # fatfs littlefs spiffs      # if you mount config.json from FS
        # esp_adc_cal                # battery monitoring
        # console                    # for debug
)