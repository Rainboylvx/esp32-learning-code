#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "key.h"
#include "led.h"

typedef enum { MODE_OFF, MODE_ON, MODE_BLINK } mode_t;

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(key_init());
    mode_t mode = MODE_OFF;
    int64_t entered_at = esp_timer_get_time();
    bool previous_on = false;
    TickType_t wake = xTaskGetTickCount();
    while (1) {
        int64_t now = esp_timer_get_time();
        // 输入事件触发状态转换；持续按住不会连续切换模式。
        if (key_poll(now)) {
            mode = (mode + 1) % 3;
            entered_at = now;
            ESP_LOGI("state", "mode=%d (0=OFF, 1=ON, 2=BLINK)", mode);
        }
        // 每 500 ms 改变状态，但不等待 500 ms。漏过节拍时按实际时间求相位。
        bool on = mode == MODE_ON ||
            (mode == MODE_BLINK && ((now - entered_at) / 500000) % 2 == 0);
        if (on != previous_on) {
            ESP_ERROR_CHECK(led_set(on));
            previous_on = on;
        }
        // 10 ms 是输入采样节拍，阻塞时 CPU 可运行其他任务。
        vTaskDelayUntil(&wake, pdMS_TO_TICKS(10));
    }
}
