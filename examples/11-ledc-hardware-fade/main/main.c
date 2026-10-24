#include "esp_log.h"
#include "esp_timer.h"
#include "key.h"
#include "fade.h"

void app_main(void)
{
    ESP_ERROR_CHECK(pwm_led_init());
    ESP_ERROR_CHECK(key_init());
    ESP_ERROR_CHECK(fade_init(xTaskGetCurrentTaskHandle()));
    bool running = true;
    uint32_t target = PWM_LED_MAX;
    ESP_ERROR_CHECK(fade_to(target, 1000));
    TickType_t wake = xTaskGetTickCount();
    while (1) {
        bool pressed = key_poll(esp_timer_get_time());
        if (pressed) {
            // 先终止旧渐变，再清除已排队的完成通知，防止误启动下一段。
            ESP_ERROR_CHECK(fade_stop());
            ulTaskNotifyTake(pdTRUE, 0);
            running = !running;
            if (running) {
                // 从暗端重新开始，避免暂停恰好落在终点时发起零距离渐变。
                ESP_ERROR_CHECK(pwm_led_set(0));
                target = PWM_LED_MAX;
                ESP_ERROR_CHECK(fade_to(target, 1000));
            }
            ESP_LOGI("fade", "%s", running ? "restart" : "paused");
        } else if (ulTaskNotifyTake(pdTRUE, 0) && running) {
            target = target == PWM_LED_MAX ? 0 : PWM_LED_MAX;
            ESP_ERROR_CHECK(fade_to(target, 1000));
        }
        // PWM 和渐变由硬件继续执行；任务每 10 ms 检查按键和完成事件。
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
