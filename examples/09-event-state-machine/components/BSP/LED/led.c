#include "led.h"
#include "driver/gpio.h"
#include "esp_check.h"
esp_err_t led_init(void)
{
    gpio_config_t cfg = {.pin_bit_mask = 1ULL << GPIO_NUM_1,
                         .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), "led", "init");
    return led_set(false);
}
esp_err_t led_set(bool on)
{
    return gpio_set_level(GPIO_NUM_1, on ? 0 : 1);
}
