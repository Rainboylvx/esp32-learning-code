#include "fade.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_attr.h"

static bool IRAM_ATTR fade_done(const ledc_cb_param_t *event, void *arg)
{
    (void)event;
    BaseType_t woken = pdFALSE;
    // 渐变完成回调在 ISR；只通知任务，由任务启动下一段渐变。
    vTaskNotifyGiveFromISR((TaskHandle_t)arg, &woken);
    return woken == pdTRUE;
}

esp_err_t fade_init(TaskHandle_t receiver)
{
    ESP_RETURN_ON_ERROR(ledc_fade_func_install(0), "fade", "install");
    ledc_cbs_t callbacks = {.fade_cb = fade_done};
    return ledc_cb_register(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                           &callbacks, receiver);
}

esp_err_t fade_to(uint32_t brightness, int duration_ms)
{
    if (brightness > PWM_LED_MAX || duration_ms <= 0) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,
        LEDC_CHANNEL_0, brightness, duration_ms), "fade", "target");
    return ledc_fade_start(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, LEDC_FADE_NO_WAIT);
}

esp_err_t fade_stop(void)
{
    // ESP32-S3 支持中止 NO_WAIT 渐变；停止后才允许设置新的占空比。
    return ledc_fade_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
