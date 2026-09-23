#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"

static const char *TAG = "led_demo";

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
