#include "pwm_led.h"
#include "driver/ledc.h"
#include "esp_check.h"

esp_err_t pwm_led_init(void)
{
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, // S3 的 LEDC 仅支持低速模式。
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0, .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer), "pwm_led", "timer");
    ledc_channel_config_t channel = {
        .gpio_num = 1, .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0, .timer_sel = LEDC_TIMER_0,
        .duty = 0, .hpoint = 0,
        // 硬件反相：内部 duty 越大，GPIO1 低电平时间越长，LED 越亮。
        .flags.output_invert = 1,
    };
    return ledc_channel_config(&channel);
}

esp_err_t pwm_led_set(uint32_t brightness)
{
    if (brightness > PWM_LED_MAX) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0,
                        brightness), "pwm_led", "set");
    return ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
