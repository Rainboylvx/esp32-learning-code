#include "soft_pwm.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"

static gptimer_handle_t s_timer;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned s_pending, s_active, s_phase;

static bool on_slot(gptimer_handle_t timer,
                    const gptimer_alarm_event_data_t *data, void *arg)
{
    (void)timer; (void)data; (void)arg;
    // 只在周期边界更新占空比，避免修改一半周期造成畸形脉冲。
    if (s_phase == 0) {
        portENTER_CRITICAL_ISR(&s_lock);
        s_active = s_pending;
        portEXIT_CRITICAL_ISR(&s_lock);
    }
    // 低电平点亮：这里的软件每 500 us 决定一次 GPIO 电平。
    gpio_set_level(GPIO_NUM_1, s_phase < s_active ? 0 : 1);
    s_phase = (s_phase + 1) % SOFT_PWM_SLOTS;
    return false; // 没有唤醒任务。
}

esp_err_t soft_pwm_set(unsigned slots_on)
{
    if (slots_on > SOFT_PWM_SLOTS) return ESP_ERR_INVALID_ARG;
    // ESP32-S3 是双核；volatile 不能代替任务与 ISR 之间的互斥。
    portENTER_CRITICAL(&s_lock);
    s_pending = slots_on;
    portEXIT_CRITICAL(&s_lock);
    return ESP_OK;
}

esp_err_t soft_pwm_init(void)
{
    gpio_config_t gpio = {.pin_bit_mask = 1ULL << GPIO_NUM_1,
                          .mode = GPIO_MODE_OUTPUT};
    ESP_RETURN_ON_ERROR(gpio_config(&gpio), "soft_pwm", "gpio");
    ESP_RETURN_ON_ERROR(gpio_set_level(GPIO_NUM_1, 1), "soft_pwm", "off");
    gptimer_config_t config = {.clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP, .resolution_hz = 1000000};
    ESP_RETURN_ON_ERROR(gptimer_new_timer(&config, &s_timer), "soft_pwm", "new");
    gptimer_event_callbacks_t cb = {.on_alarm = on_slot};
    ESP_RETURN_ON_ERROR(gptimer_register_event_callbacks(s_timer, &cb, NULL),
                        "soft_pwm", "callback");
    // 500 us / 槽 × 20 槽 = 10 ms / 周期，即 100 Hz PWM。
    gptimer_alarm_config_t alarm = {.alarm_count = 500, .reload_count = 0,
                                   .flags.auto_reload_on_alarm = true};
    ESP_RETURN_ON_ERROR(gptimer_set_alarm_action(s_timer, &alarm), "soft_pwm", "alarm");
    ESP_RETURN_ON_ERROR(gptimer_enable(s_timer), "soft_pwm", "enable");
    return gptimer_start(s_timer);
}
