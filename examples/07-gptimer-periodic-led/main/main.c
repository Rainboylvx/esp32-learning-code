#include <inttypes.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "led.h"
#include "tick.h"

void app_main(void)
{
    ESP_ERROR_CHECK(led_init());
    ESP_ERROR_CHECK(tick_start(xTaskGetCurrentTaskHandle()));
    bool on = false;
    int64_t previous = esp_timer_get_time();
    while (1) {
        // 阻塞让出 CPU；通知计数保留“任务暂时没空”期间收到的次数。
        uint32_t count = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (count & 1U) on = !on;
        ESP_ERROR_CHECK(led_set(on));
        int64_t now = esp_timer_get_time();
        ESP_LOGI("gptimer", "alarms=%" PRIu32 ", task interval=%" PRId64
                 " us, LED=%d", count, now - previous, on);
        previous = now;
    }
}
