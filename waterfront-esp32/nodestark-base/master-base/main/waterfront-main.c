/*
 * Waterfront Kayak Rental Controller – Bremen, DE
 * MDB / vending protocol completely removed
 * Purpose: MQTT-based remote solenoid lock control + ultrasonic return sensor + battery monitoring
 * Original base: Nodestark/mdb-esp32-cashless master mode (adapted)
 */

#include <esp_log.h>

#include <string.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <driver/gpio.h>
#include <driver/uart.h>
#include <driver/gpio.h>
#include <rom/ets_sys.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "led_strip.h"

#define TAG "kayak_rental"

void app_main(void) {

    //--------------- Strip LED configuration ---------------//
    led_strip_handle_t led_strip;
    led_strip_config_t strip_config = {
        .strip_gpio_num = GPIO_NUM_2,
        .max_leds = 1,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags.invert_out = false,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz → good precision
        .mem_block_symbols = 64,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));

    //
}
