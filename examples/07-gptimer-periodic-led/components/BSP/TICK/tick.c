#include "tick.h"
#include "driver/gptimer.h"
#include "esp_check.h"

static gptimer_handle_t s_timer;

static bool on_alarm(gptimer_handle_t timer,
                     const gptimer_alarm_event_data_t *event, void *arg)
{
    (void)timer;
    (void)event;
    BaseType_t woken = pdFALSE;
    // GPTimer 回调运行在 ISR，只能使用 FromISR 版本。
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &woken);
    // 由驱动根据返回值请求切换任务，无需再次 portYIELD_FROM_ISR。
    return woken == pdTRUE;
}

esp_err_t tick_start(TaskHandle_t receiver)
{
    if (receiver == NULL) return ESP_ERR_INVALID_ARG;
    gptimer_config_t config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000, // 每计数一次经过 1 us。
    };
    ESP_RETURN_ON_ERROR(gptimer_new_timer(&config, &s_timer), "tick", "new");
    gptimer_event_callbacks_t callbacks = {.on_alarm = on_alarm};
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(s_timer, &callbacks,
                         receiver), "tick", "callback");
    gptimer_alarm_config_t alarm = {
        .alarm_count = 1000000, // (1000000 - 0) / 1000000 Hz = 1 s。
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(s_timer, &alarm), "tick", "alarm");
    ESP_RETURN_ON_ERROR(gptimer_enable(s_timer), "tick", "enable");
    return gptimer_start(s_timer);
}
