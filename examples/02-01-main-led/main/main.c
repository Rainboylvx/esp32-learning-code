#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LED_GPIO GPIO_NUM_1

static const char *TAG = "led_demo";

static esp_err_t led_set(bool on)
{
    return gpio_set_level(LED_GPIO, on ? 0 : 1);
}

static esp_err_t led_init(void)
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

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());

    while (1) {
        ESP_ERROR_CHECK(led_set(true));
        ESP_LOGI(TAG, "LED on");
        vTaskDelay(pdMS_TO_TICKS(500));

        ESP_ERROR_CHECK(led_set(false));
        ESP_LOGI(TAG, "LED off");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
