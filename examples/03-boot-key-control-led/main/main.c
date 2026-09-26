#include <stdbool.h>
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "key.h"
#include "led.h"

static const char *TAG = "key_demo";

void app_main(void)
{
    // LED 初始化后默认熄灭；应用层保存的是“亮/灭”这个逻辑状态。
    bool led_on = false;

    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(key_init());
    ESP_LOGI(TAG, "Ready: press BOOT to toggle LED");

    while (1) {
        // key_scan() 只在一次经过消抖确认的新按下动作发生时返回事件。
        if (key_scan() == KEY_EVENT_BOOT_PRESS) {
            led_on = !led_on;
            ESP_ERROR_CHECK(led_set(led_on));
            ESP_LOGI(TAG, "BOOT pressed, LED %s", led_on ? "on" : "off");
        }

        // 限制轮询频率，让其他 FreeRTOS 任务有机会运行。
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
