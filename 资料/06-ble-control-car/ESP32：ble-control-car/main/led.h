#pragma once

#if CONFIG_LED_SETUP
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
#include "esp_log.h"
#endif

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "led_strip.h"

#define TAG "CAR_LED"

#if CONFIG_BOARD_TYPE_C3
// 左灯
void set_led_left(int v)
{
    gpio_set_level(CONFIG_LED_PIN2, v);
}

// 右灯
void set_led_right(int v)
{
    gpio_set_level(CONFIG_LED_PIN1, v);
}

void led_init(void)
{
    // 初始化引脚
    gpio_reset_pin(CONFIG_LED_PIN1);
    gpio_set_direction(CONFIG_LED_PIN1, GPIO_MODE_OUTPUT);

    gpio_reset_pin(CONFIG_LED_PIN2);
    gpio_set_direction(CONFIG_LED_PIN2, GPIO_MODE_OUTPUT);

    gpio_set_level(CONFIG_LED_PIN1, 0);
    gpio_set_level(CONFIG_LED_PIN2, 0);
}
#endif

#if CONFIG_BOARD_TYPE_S3
led_strip_handle_t led_strip;

void led_init(void)
{
    gpio_reset_pin(CONFIG_LED_PIN);
    gpio_set_direction(CONFIG_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_pull_mode(CONFIG_LED_PIN, GPIO_FLOATING);

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_LED_PIN,
        .max_leds = CONFIG_MAX_LEDS,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // WS2812通常是GRB顺序
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
        .flags.with_dma = false,
    };

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#if CONFIG_LOG_DEFAULT_LEVEL_INFO || CONFIG_LOG_DEFAULT_LEVEL_WARN || CONFIG_LOG_DEFAULT_LEVEL_ERROR
    ESP_LOGI(TAG, "LED strip initialized on GPIO %d", CONFIG_LED_PIN);
#endif
    // 清除LED
    ESP_ERROR_CHECK(led_strip_clear(led_strip));
}

// 设置颜色（带亮度控制）
void set_led_with_brightness(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    // 调整亮度
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;

    led_strip_set_pixel(led_strip, 0, r, g, b);
    led_strip_refresh(led_strip);
}

// 常用的颜色函数
void set_led_cyan(void)
{
    set_led_with_brightness(0, 255, 255, 50);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_strip_clear(led_strip);
}

void set_led_red(void)
{
    set_led_with_brightness(255, 0, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_strip_clear(led_strip);
}

void set_led_blue(bool auto_close)
{
    set_led_with_brightness(0, 0, 255, 50);
    if (auto_close)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        led_strip_clear(led_strip);
    }
}

void set_led_green(void)
{
    set_led_with_brightness(0, 255, 0, 50);
    vTaskDelay(pdMS_TO_TICKS(1000));
    led_strip_clear(led_strip);
}
#endif
#endif