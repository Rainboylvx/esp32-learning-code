#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led.h"

// 应用层只负责“何时亮、何时灭”，板卡接线细节由 BSP 负责。
static const char *TAG = "led_demo";

void app_main(void)
{
    // 应用层通过 BSP 接口初始化 LED，无需直接调用 gpio_config()。
    ESP_ERROR_CHECK(led_init());

    while (1) {
        // true 表示“点亮”。BSP 会把它转换成这块板需要的低电平。
        ESP_ERROR_CHECK(led_set(true));
        ESP_LOGI(TAG, "LED on");
        vTaskDelay(pdMS_TO_TICKS(500));

        // false 表示“熄灭”，应用层不需要记忆高低电平的对应关系。
        ESP_ERROR_CHECK(led_set(false));
        ESP_LOGI(TAG, "LED off");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
