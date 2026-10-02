#include "led.h"
#include "driver/gpio.h"

// DNESP32S3 V1.2 的板载红色 LED 接 GPIO1，低电平点亮。
#define LED_GPIO GPIO_NUM_1

esp_err_t led_init(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = 1ULL << LED_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    esp_err_t err = gpio_config(&config);
    if (err != ESP_OK) {
        return err;
    }
    return led_set(false);
}

esp_err_t led_set(bool on)
{
    return gpio_set_level(LED_GPIO, on ? 0 : 1);
}
