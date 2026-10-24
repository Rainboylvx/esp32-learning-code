#include "soft_pwm.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{
    ESP_ERROR_CHECK(soft_pwm_init());
    const unsigned levels[] = {0, 5, 10, 15, 20};
    while (1) {
        for (unsigned i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i) {
            ESP_ERROR_CHECK(soft_pwm_set(levels[i]));
            ESP_LOGI("soft_pwm", "logical brightness=%u%%", levels[i] * 5);
            // 这里只控制示范档位的停留时间。PWM 本身在中断中持续输出。
            vTaskDelay(pdMS_TO_TICKS(1500));
        }
    }
}
