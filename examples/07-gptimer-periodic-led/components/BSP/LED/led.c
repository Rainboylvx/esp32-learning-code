#include "led.h"
#include "esp_check.h"
#include "driver/gpio.h"

esp_err_t led_init(void)
{
    // DNESP32S3 V1.2：GPIO1 输出低电平，红色 LED 点亮。
    gpio_config_t config = {.pin_bit_mask = 1ULL << GPIO_NUM_1,
                            .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&config), "led", "GPIO init");
    return led_set(false);
}

esp_err_t led_set(bool on)
{
    return gpio_set_level(GPIO_NUM_1, on ? 0 : 1);
}
